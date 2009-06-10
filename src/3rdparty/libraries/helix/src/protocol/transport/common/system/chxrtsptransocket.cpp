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
#include "hxcom.h"

#include "debug.h"
#include "hxassert.h"
#include "hxheap.h"
#include "hxnet.h" 
#include "chxrtsptransocket.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

CHXRTSPTranSocket::CHXRTSPTranSocket()
{
    m_pSock = NULL;
    m_pResponse = NULL;
    m_nRefCount = 0;
}

CHXRTSPTranSocket::CHXRTSPTranSocket(IHXSocket* pSocket)
    : m_nRefCount(0)
    , m_pSock(0)
    , m_pResponse(0)
{
    HX_ASSERT(pSocket);
    m_pSock = pSocket;
    m_pSock->AddRef();
}

CHXRTSPTranSocket::~CHXRTSPTranSocket(void)
{
    HX_RELEASE(m_pSock);
    HX_RELEASE(m_pResponse);
}

STDMETHODIMP
CHXRTSPTranSocket::QueryInterface(REFIID riid, void** ppvObj)
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
CHXRTSPTranSocket::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXRTSPTranSocket::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = (INT32)InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return (ULONG32)rc;
}

STDMETHODIMP_(HXSockFamily)
CHXRTSPTranSocket::GetFamily(void)
{
    return m_pSock->GetFamily();
}

STDMETHODIMP_(HXSockType)
CHXRTSPTranSocket::GetType(void)
{
    return m_pSock->GetType();
}

STDMETHODIMP_(HXSockProtocol)
CHXRTSPTranSocket::GetProtocol(void)
{
    return m_pSock->GetProtocol();
}

STDMETHODIMP
CHXRTSPTranSocket::Init(HXSockFamily f, HXSockType t, HXSockProtocol p)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXRTSPTranSocket::SetResponse(IHXSocketResponse* pResponse)
{
    HX_RELEASE(m_pResponse);
    m_pResponse = pResponse;
    if(m_pResponse)
    {
        m_pResponse->AddRef();
    }

    return HXR_OK;
}

STDMETHODIMP
CHXRTSPTranSocket::CreateSockAddr(IHXSockAddr** ppAddr)
{
    return m_pSock->CreateSockAddr(ppAddr);
}

STDMETHODIMP
CHXRTSPTranSocket::Bind(IHXSockAddr* pAddr)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXRTSPTranSocket::ConnectToOne(IHXSockAddr* pAddr)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXRTSPTranSocket::ConnectToAny(UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXRTSPTranSocket::GetLocalAddr(IHXSockAddr** ppAddr)
{
    return m_pSock->GetLocalAddr(ppAddr);
}

STDMETHODIMP
CHXRTSPTranSocket::GetPeerAddr(IHXSockAddr** ppAddr)
{
    return m_pSock->GetPeerAddr(ppAddr);
}

STDMETHODIMP
CHXRTSPTranSocket::SelectEvents(UINT32 uEventMask)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXRTSPTranSocket::Peek(IHXBuffer** pBuf)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXRTSPTranSocket::Read(IHXBuffer** pBuf)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXRTSPTranSocket::Write(IHXBuffer* pBuf)
{
    return m_pSock->Write(pBuf);
}

STDMETHODIMP
CHXRTSPTranSocket::Close(void)
{
    HX_RELEASE(m_pResponse);
    return HXR_OK;
}

STDMETHODIMP
CHXRTSPTranSocket::Listen(UINT32 uBackLog)
{

    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXRTSPTranSocket::Accept(IHXSocket** ppNewSock, IHXSockAddr** ppSource)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXRTSPTranSocket::GetOption(HXSockOpt name, UINT32* pval)
{
    return m_pSock->GetOption(name, pval);
}

STDMETHODIMP
CHXRTSPTranSocket::SetOption(HXSockOpt name, UINT32 val)
{
    if (HX_SOCKOPT_TCP_NODELAY == name)
    {
        return m_pSock->SetOption(name, val);
    }
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXRTSPTranSocket::PeekFrom(IHXBuffer** pBuf, IHXSockAddr** pAddr)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXRTSPTranSocket::ReadFrom(IHXBuffer** pBuf, IHXSockAddr** pAddr)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXRTSPTranSocket::WriteTo(IHXBuffer* pBuf, IHXSockAddr* pAddr)
{
    return m_pSock->WriteTo(pBuf, pAddr);
}

STDMETHODIMP
CHXRTSPTranSocket::ReadV(UINT32 nVecLen, UINT32* puLenVec, IHXBuffer** ppVec)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXRTSPTranSocket::ReadFromV(UINT32 nVecLen, UINT32* puLenVec, IHXBuffer** ppVec, IHXSockAddr** pAddr)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXRTSPTranSocket::WriteV(UINT32 nVecLen, IHXBuffer** ppVec)
{
    return m_pSock->WriteV(nVecLen, ppVec);
}

STDMETHODIMP
CHXRTSPTranSocket::WriteToV(UINT32 nVecLen, IHXBuffer** ppVec, IHXSockAddr* pAddr)
{
    return m_pSock->WriteToV(nVecLen, ppVec, pAddr);
}

STDMETHODIMP
CHXRTSPTranSocket::EventPending(UINT32 uEvent, HX_RESULT status)
{
    return m_pResponse->EventPending(uEvent, status);
}
