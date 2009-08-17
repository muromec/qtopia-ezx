/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: servlbsock.cpp,v 1.6 2006/01/04 20:36:54 ckarusala Exp $
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
#include "nettypes.h"
#include "hxcom.h"
#include "server_context.h"
#include "servreg.h"
#include "hxnet.h"
#include "servlbsock.h"

#include "lbound_listenresp.h"

#include "servbuffer.h"
#include "hxsbuffer.h"

/* XXXTDM
 * The lbsock write queue is currently implemented as a linked list of buffer
 * objects with a maximum element size.  This design exhibits the worst traits
 * of vectors and lists: it is bounded in size like a vector and uses dynamic
 * allocation to store elements like a list.  The ideal solution is probably
 * to adapt the CHXStreamWriteQueue class to allow buffer retrieval and use it
 * here.  This would provide good buffer management without duplicating code.
 */
static const UINT32 z_ulMaxLBWriteQueueSize = 100;

CHXSockAddrLB::CHXSockAddrLB(void* pSock) :
    m_nRefCount(0),
    m_pSock(pSock)
{
    // Empty
};

STDMETHODIMP
CHXSockAddrLB::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXSockAddr*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSockAddr))
    {
        AddRef();
        *ppvObj = (IHXSockAddr*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXSockAddrLB::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXSockAddrLB::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP_(HXSockFamily)
CHXSockAddrLB::GetFamily(void)
{
    return HX_SOCK_FAMILY_LOCAL;
}

STDMETHODIMP_(BOOL)
CHXSockAddrLB::IsEqual(IHXSockAddr* pOther)
{
    return FALSE;
}

STDMETHODIMP
CHXSockAddrLB::Copy(IHXSockAddr* pOther)
{
    return HXR_FAIL;
}

STDMETHODIMP
CHXSockAddrLB::Clone(IHXSockAddr** ppNew)
{
    return HXR_FAIL;
}

STDMETHODIMP
CHXSockAddrLB::GetAddr(IHXBuffer** ppBuf)
{
    char szAddr[80];
    size_t len;

    *ppBuf = new ServerBuffer;
    (*ppBuf)->AddRef();

    sprintf(szAddr, "lbound@%p", m_pSock);
    len = strlen(szAddr)+1;
    (*ppBuf)->SetSize(len);
    memcpy((*ppBuf)->GetBuffer(), szAddr, len);
    return HXR_OK;
}

STDMETHODIMP
CHXSockAddrLB::SetAddr(IHXBuffer* pBuf)
{
    HX_ASSERT(m_nRefCount == 1);
    return HXR_FAIL;
}

STDMETHODIMP_(BOOL)
CHXSockAddrLB::IsEqualAddr(IHXSockAddr* pOther)
{
    return IsEqual(pOther);
}

STDMETHODIMP_(UINT16)
CHXSockAddrLB::GetPort(void)
{
    HX_ASSERT(FALSE);
    return 0;
}

STDMETHODIMP
CHXSockAddrLB::SetPort(UINT16 port)
{
    HX_ASSERT(m_nRefCount == 1);
    HX_ASSERT(FALSE);
    return HXR_NOTIMPL;
}

STDMETHODIMP_(BOOL)
CHXSockAddrLB::IsEqualPort(IHXSockAddr* pOther)
{
    HX_ASSERT(FALSE);
    return FALSE;
}

STDMETHODIMP
CHXSockAddrLB::MaskAddr(UINT32 nBits)
{
    HX_ASSERT(m_nRefCount == 1);
    HX_ASSERT(FALSE);
    return HXR_NOTIMPL;
}

STDMETHODIMP_(BOOL)
CHXSockAddrLB::IsEqualNet(IHXSockAddr* pOther, UINT32 nBits)
{
    HX_ASSERT(FALSE);
    return FALSE;
}

CLBSocket::CLBSocket(Process* pProc)
    : m_ulRefCount(0)
    , m_ulSelectedEvents(0)
    , m_ulAllowedEvents(0)
    , m_ulPendingEvents(0)
    , m_CallbackHandle(0)
    , m_pPeer(NULL)
    , m_pScheduler((IHXScheduler*)pProc->pc->scheduler)
    , m_pResponse(NULL)
    , m_pProc(pProc)
    , m_uMaxPacketQueueSize(z_ulMaxLBWriteQueueSize)
{
    HX_ADDREF(m_pScheduler);
}

CLBSocket::CLBSocket(Process* pProc, CLBSocket* pPeer)
    : m_ulRefCount(0)
    , m_ulSelectedEvents(0)
    , m_ulAllowedEvents(0)
    , m_ulPendingEvents(0)
    , m_CallbackHandle(0)
    , m_pPeer(pPeer)
    , m_pScheduler((IHXScheduler*)pProc->pc->scheduler)
    , m_pResponse(NULL)
    , m_pProc(pProc)
    , m_uMaxPacketQueueSize(z_ulMaxLBWriteQueueSize)
{
    HX_ADDREF(m_pPeer);
    HX_ADDREF(m_pScheduler);
}

CLBSocket::~CLBSocket(void)
{
    if (m_CallbackHandle)
    {
        m_pScheduler->Remove(m_CallbackHandle);
    }
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pPeer);
}

STDMETHODIMP
CLBSocket::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSocket))
    {
        AddRef();
        *ppvObj = (IHXSocket*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CLBSocket::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CLBSocket::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_ulRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP_(HXSockFamily)
CLBSocket::GetFamily(void)
{
    return HX_SOCK_FAMILY_LBOUND;
}

STDMETHODIMP_(HXSockType)
CLBSocket::GetType(void)
{
    // We're faking TCP, here.
    return HX_SOCK_TYPE_TCP;
}

STDMETHODIMP_(HXSockProtocol)
CLBSocket::GetProtocol(void)
{
    // Don't know if this should be called
    HX_ASSERT(FALSE);
    return (HXSockProtocol)(-1);
}

STDMETHODIMP
CLBSocket::Init(HXSockFamily f, HXSockType t, HXSockProtocol p)
{
    return HXR_OK;
}

STDMETHODIMP
CLBSocket::SetResponse(IHXSocketResponse* pResponse)
{
    IHXSocketResponse* pTemp = m_pResponse;
    m_pResponse = pResponse;

    HX_ADDREF(m_pResponse);
    HX_RELEASE(pTemp);
    return HXR_OK;
}

STDMETHODIMP
CLBSocket::SetAccessControl(IHXSocketAccessControl* pControl)
{
    // Don't know if this should be called
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CLBSocket::CreateSockAddr(IHXSockAddr** ppAddr)
{
    // Don't know if this should be called
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CLBSocket::Bind(IHXSockAddr* pAddr)
{
    // Don't know if this should be called
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CLBSocket::ConnectToOne(IHXSockAddr* pAddr)
{
    // Don't pass in an addr, it is implied
    HX_ASSERT(pAddr == NULL);

    if (m_pPeer != NULL)
    {
        //XXXDPL return HX_SOCKERR_ALREADY;
        return HXR_FAIL;
    }

    m_pPeer = new CLBSocket(m_pProc, this);
    m_pPeer->AddRef();
    m_pProc->pc->lbound_tcp_listenRTSPResponse->OnConnection(m_pPeer, m_pProc);
    ConnectComplete();

    return HXR_OK;
}

STDMETHODIMP
CLBSocket::ConnectToAny(UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    // Don't call this
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CLBSocket::GetLocalAddr(IHXSockAddr** ppAddr)
{
    *ppAddr = new CHXSockAddrLB(this);
    (*ppAddr)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
CLBSocket::GetPeerAddr(IHXSockAddr** ppAddr)
{
    if (m_pPeer == NULL)
    {
        return HXR_FAIL;
    }
    *ppAddr = new CHXSockAddrLB(m_pPeer);
    (*ppAddr)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
CLBSocket::SelectEvents(UINT32 ulEventMask)
{
    m_ulSelectedEvents = ulEventMask;
    m_ulAllowedEvents = m_ulSelectedEvents;

    if (m_ulAllowedEvents & HX_SOCK_EVENT_READ)
    {
        if (m_pPeer && !m_pPeer->m_PacketQueue.IsEmpty())
        {
            ScheduleReadCallback();
        }
    }

    if (m_ulAllowedEvents & HX_SOCK_EVENT_WRITE)
    {
        if (m_PacketQueue.GetCount() < m_uMaxPacketQueueSize)
        {
            ScheduleWriteCallback();
        }
    }

    return HXR_OK;
}

STDMETHODIMP
CLBSocket::Peek(IHXBuffer** ppBuf)
{
    if (!m_pPeer || m_pPeer->m_PacketQueue.IsEmpty())
    {
        return HXR_FAIL;
    }

    *ppBuf = (IHXBuffer*)m_pPeer->m_PacketQueue.GetHead();
    (*ppBuf)->AddRef();
    HX_ASSERT((*ppBuf)->GetSize() > 0);

    return HXR_OK;
}

STDMETHODIMP
CLBSocket::Read(IHXBuffer** ppBuf)
{
    if (!m_pPeer)
    {
        //XXXDPL more accurate return value?
        return HXR_FAIL;
    }
    if (m_pPeer->m_PacketQueue.IsEmpty())
    {
        //XXXDPL more accurate return value?
        return HXR_FAIL;
    }

    *ppBuf = (IHXBuffer*)m_pPeer->m_PacketQueue.RemoveHead();
    HX_ASSERT((*ppBuf)->GetSize() > 0);

    if ((m_pPeer->m_ulAllowedEvents & m_pPeer->m_ulSelectedEvents & HX_SOCK_EVENT_WRITE)
    &&  m_pPeer->m_pResponse)
    {
        m_pPeer->ScheduleWriteCallback();
    }

    m_ulAllowedEvents |= HX_SOCK_EVENT_READ;

    // Should check for allowed too, but that's obviously true.
    if ((m_ulSelectedEvents & HX_SOCK_EVENT_READ)
    &&  !m_pPeer->m_PacketQueue.IsEmpty())
    {
        ScheduleReadCallback();
    }

    return HXR_OK;
}

STDMETHODIMP
CLBSocket::Write(IHXBuffer* pBuf)
{
    if (!m_pPeer)
    {
        //XXXDPL return HX_SOCKERR_NOTCONN;
        return HXR_FAIL;
    }

    if (!pBuf)
    {
        return HXR_INVALID_PARAMETER;
    }

    if (pBuf->GetSize() == 0)
    {
        return HXR_OK;
    }

    if (m_PacketQueue.GetCount() >= m_uMaxPacketQueueSize)
    {
        m_ulAllowedEvents |= HX_SOCK_EVENT_WRITE;

        return HXR_SOCK_WOULDBLOCK;
    }

    pBuf->AddRef();
    m_PacketQueue.AddTail(pBuf);

    if (!m_PacketQueue.IsEmpty()
    &&  (m_pPeer->m_ulSelectedEvents & m_pPeer->m_ulAllowedEvents & HX_SOCK_EVENT_READ)
    &&  m_pPeer->m_pResponse)
    {
        m_pPeer->m_pResponse->EventPending(HX_SOCK_EVENT_READ, HXR_OK);
    }

    if (m_PacketQueue.IsEmpty())
    {
    return HXR_OK;
}
    else
    {
        m_ulAllowedEvents |= HX_SOCK_EVENT_WRITE;
	return HXR_SOCK_BUFFERED;
    }
}

STDMETHODIMP
CLBSocket::Close(void)
{
    HX_RELEASE(m_pResponse);
    if (m_pPeer != NULL && (m_pPeer->m_ulSelectedEvents & HX_SOCK_EVENT_CLOSE))
    {
        // Should never be disallowed.
        HX_ASSERT(m_pPeer->m_ulAllowedEvents & HX_SOCK_EVENT_CLOSE);

        // Save peer's response and then release peer to signify closure
        IHXSocketResponse* pPeerResponse = m_pPeer->m_pResponse;
        HX_RELEASE(m_pPeer);

        if (pPeerResponse != NULL)
        {
            pPeerResponse->EventPending(HX_SOCK_EVENT_CLOSE, HXR_OK);
        }
    }

    while (!m_PacketQueue.IsEmpty())
    {
        IHXBuffer* pBuf = (IHXBuffer*)m_PacketQueue.RemoveHead();
        HX_RELEASE(pBuf);
    }

    return HXR_OK;
}

STDMETHODIMP
CLBSocket::Listen(UINT32 uBackLog)
{
    // Don't know if this should be called
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CLBSocket::Accept(IHXSocket** ppNewSock, IHXSockAddr** ppSource)
{
    // Don't know if this should be called
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CLBSocket::GetOption(HXSockOpt name, UINT32* pval)
{
    switch (name)
    {
    case HX_SOCKOPT_APP_SNDBUF:
        *pval = m_uMaxPacketQueueSize;
        return HXR_OK;
    default:
        HX_ASSERT(FALSE);
    }
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CLBSocket::SetOption(HXSockOpt name, UINT32 val)
{
    switch (name)
    {
    case HX_SOCKOPT_APP_SNDBUF:
        m_uMaxPacketQueueSize = val;
        return HXR_OK;
    case HX_SOCKOPT_LINGER:
        return HXR_NOTIMPL;
    default:
        HX_ASSERT(FALSE);
    }
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CLBSocket::PeekFrom(IHXBuffer** ppBuf, IHXSockAddr** ppAddr)
{
    // lbound is TCP, this should not be called
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CLBSocket::ReadFrom(IHXBuffer** ppBuf, IHXSockAddr** ppAddr)
{
    // lbound is TCP, this should not be called
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CLBSocket::WriteTo(IHXBuffer* pBuf, IHXSockAddr* pAddr)
{
    // lbound is TCP, this should not be called
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CLBSocket::ReadV(UINT32 nVecLen, UINT32* puLenVec,
                IHXBuffer** ppBufVec)
{
    // XXX: implement me
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CLBSocket::ReadFromV(UINT32 nVecLen, UINT32* puLenVec,
                IHXBuffer** ppBufVec, IHXSockAddr** ppAddr)
{
    // lbound is TCP, this should not be called
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

STDMETHODIMP_(INT32)
CLBSocket::WriteV(UINT32 nVecLen, IHXBuffer** ppBufVec)
{
    //XXXDPLHACK - No one else but RTSPServerProt should be using
    //lbound sockets, so this should be ok.
    HX_RESULT hr = HXR_OK;
    for (UINT32 i = 0; i < nVecLen; i++)
    {
	// it is possible that Write() returns HXR_SOCK_BUFFERED here.  
        if (FAILED(hr = Write(ppBufVec[i])))
        {
            break;
        }
    }
    return hr;
}

STDMETHODIMP_(INT32)
CLBSocket::WriteToV(UINT32 nVecLen, IHXBuffer** ppBufVec,
                IHXSockAddr* pAddr)
{
    // lbound is TCP, this should not be called
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CLBSocket::Func(void)
{
    m_CallbackHandle = 0;
    if (!m_pResponse)
    {
        return HXR_OK;
    }

    if (m_ulPendingEvents & HX_SOCK_EVENT_READ)
    {
        m_ulPendingEvents &= ~HX_SOCK_EVENT_READ;
        m_ulAllowedEvents &= ~HX_SOCK_EVENT_READ;
        m_pResponse->EventPending(HX_SOCK_EVENT_READ, HXR_OK);
    }
    if (m_ulPendingEvents & HX_SOCK_EVENT_WRITE)
    {
        m_ulPendingEvents &= ~HX_SOCK_EVENT_WRITE;
        m_ulAllowedEvents &= ~HX_SOCK_EVENT_WRITE;
        m_pResponse->EventPending(HX_SOCK_EVENT_WRITE, HXR_OK);
    }
    return HXR_OK;
}

void
CLBSocket::ScheduleReadCallback(void)
{
    HX_ASSERT(m_ulSelectedEvents & HX_SOCK_EVENT_READ);
    HX_ASSERT(m_ulAllowedEvents & HX_SOCK_EVENT_READ);

    m_ulPendingEvents |= HX_SOCK_EVENT_READ;
    if (!m_CallbackHandle)
    {
        m_CallbackHandle = m_pScheduler->RelativeEnter(this, 0);
    }
}

void
CLBSocket::ScheduleWriteCallback(void)
{
    HX_ASSERT(m_ulSelectedEvents & HX_SOCK_EVENT_READ);
    HX_ASSERT(m_ulAllowedEvents & HX_SOCK_EVENT_READ);

    m_ulPendingEvents |= HX_SOCK_EVENT_WRITE;
    if (!m_CallbackHandle)
    {
        m_CallbackHandle = m_pScheduler->RelativeEnter(this, 0);
    }
}

void
CLBSocket::ConnectComplete(void)
{
    if ((m_ulSelectedEvents & m_ulAllowedEvents & HX_SOCK_EVENT_CONNECT)
    &&  m_pResponse)
    {
        m_pResponse->EventPending(HX_SOCK_EVENT_CONNECT, HXR_OK);
    }
}
