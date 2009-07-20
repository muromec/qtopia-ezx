/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cloak_common.cpp,v 1.14 2009/05/13 21:03:48 dcollins Exp $
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

#include "hlxclib/signal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxprefs.h"
#include "hxstring.h"
#include "hxmarsh.h"
#include "mutex.h"
#include "chxpckts.h"
#include "rtsputil.h"
#include "callback_container.h"
#include "server_engine.h"
#include "debug.h"
#include "client.h"
#include "hxstrutl.h"
#include "tsmap.h"
#include "netbyte.h"
#include "servbuffer.h"
#include "hxnet.h"
#include "servsockimp.h"
#include "hxprot.h"
#include "rtspserv.h"
#include "http_demux.h"
#include "cloak_common.h"
#include "rn1cloak.h"


CloakConn::CloakConn(void) 
: m_nRefCount(0)
, m_pRTSPServProt(NULL)
, m_pRTSPSvrProt(NULL)
, m_pRefSock(NULL)
, m_pGETHandler(NULL) 
, m_ulCloakMode(0)
, m_nProcNum(0)
{
    m_pPOSTHandlersLock = HXCreateMutex();
}

CloakConn::~CloakConn(void)
{
    HX_RELEASE(m_pRTSPServProt);
    HX_RELEASE(m_pGETHandler);
    HX_RELEASE(m_pRTSPSvrProt);
    HX_RELEASE(m_pRefSock);
    HXDestroyMutex(m_pPOSTHandlersLock);
}

STDMETHODIMP
CloakConn::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSocket))
    {
        AddRef();
        *ppvObj = (IHXSocket*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CloakConn::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CloakConn::Release(void)
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
CloakConn::GetFamily(void)
{
    HX_ASSERT(m_pRTSPServProt);
    HX_ASSERT(m_pRefSock);
    return HX_SOCK_FAMILY_CLOAK;
}

STDMETHODIMP_(HXSockType)
CloakConn::GetType(void)
{
    HX_ASSERT(m_pRTSPServProt);
    HX_ASSERT(m_pRefSock);
    return m_pRefSock->GetType();
}

STDMETHODIMP_(HXSockProtocol)
CloakConn::GetProtocol(void)
{
    HX_ASSERT(m_pRTSPServProt);
    HX_ASSERT(m_pRefSock);
    return m_pRefSock->GetProtocol();
}

STDMETHODIMP
CloakConn::Init(HXSockFamily f, HXSockType t, HXSockProtocol p)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CloakConn::SetResponse(IHXSocketResponse* pResponse)
{
    HX_RELEASE(m_pRTSPSvrProt);
    m_pRTSPSvrProt = pResponse;
    HX_ADDREF(m_pRTSPSvrProt);
    return HXR_OK;
}

STDMETHODIMP
CloakConn::SetAccessControl(IHXSocketAccessControl* pControl)
{
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CloakConn::CreateSockAddr(IHXSockAddr** ppAddr)
{
    //HX_ASSERT(m_pRTSPServProt);
    HX_ASSERT(m_pRefSock);
    return m_pRefSock->CreateSockAddr(ppAddr);
}

STDMETHODIMP
CloakConn::Bind(IHXSockAddr* pAddr)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CloakConn::ConnectToOne(IHXSockAddr* pAddr)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CloakConn::ConnectToAny(UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CloakConn::GetLocalAddr(IHXSockAddr** ppAddr)
{
    HX_ASSERT(m_pRTSPServProt);
    HX_ASSERT(m_pRefSock);
    return m_pRefSock->GetLocalAddr(ppAddr);
}

STDMETHODIMP
CloakConn::GetPeerAddr(IHXSockAddr** ppAddr)
{
    HX_ASSERT(m_pRTSPServProt);
    HX_ASSERT(m_pRefSock);
    return m_pRefSock->GetPeerAddr(ppAddr);
}

STDMETHODIMP
CloakConn::SelectEvents(UINT32 uEventMask)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CloakConn::Peek(IHXBuffer** pBuf)
{
    *pBuf = (IHXBuffer*)m_ReadQueue.GetTail();

    if (!(*pBuf))
    {
        return HXR_FAIL;
    }

    (*pBuf)->AddRef();
    return HXR_OK;
}

STDMETHODIMP
CloakConn::Read(IHXBuffer** pBuf)
{
    *pBuf = (IHXBuffer*)m_ReadQueue.RemoveTail();

    if (!(*pBuf))
    {
        return HXR_FAIL;
    }

    // The reference held by the queue is 'passed' to the response obj.
    //(*pBuf)->AddRef();
    //HX_RELEASE(*pBuf);
    return HXR_OK;
}

STDMETHODIMP
CloakConn::Write(IHXBuffer* pBuf)
{
    HX_RESULT hr = HXR_UNEXPECTED;
    if (m_pGETHandler)
    {
        hr = m_pGETHandler->SendData(pBuf);
    }

    return hr;
}

STDMETHODIMP
CloakConn::Close(void)
{
    if (m_pRTSPSvrProt)
    {
        m_pRTSPSvrProt->EventPending(HX_SOCK_EVENT_CLOSE, HXR_OK);
    }

    if (m_pGETHandler)
    {
        m_pGETHandler->OnClosed();
    }

    // Must do additional if check in case close handlers NULL out these values.
    HX_RELEASE(m_pRTSPSvrProt);
    HX_RELEASE(m_pGETHandler);

    return HXR_OK;
}

STDMETHODIMP
CloakConn::Listen(UINT32 uBackLog)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CloakConn::Accept(IHXSocket** ppNewSock, IHXSockAddr** ppSource)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CloakConn::GetOption(HXSockOpt name, UINT32* pval)
{
    return m_pRefSock->GetOption(name, pval);
}

STDMETHODIMP
CloakConn::SetOption(HXSockOpt name, UINT32 val)
{
    if (HX_SOCKOPT_TCP_NODELAY == name)
    {
        return m_pRefSock->SetOption(name, val);
    }
    return HXR_NOTIMPL;
}

STDMETHODIMP
CloakConn::PeekFrom(IHXBuffer** pBuf, IHXSockAddr** pAddr)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CloakConn::ReadFrom(IHXBuffer** pBuf, IHXSockAddr** pAddr)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CloakConn::WriteTo(IHXBuffer* pBuf, IHXSockAddr* pAddr)
{
    // TCP sockets must be connected!
    return HXR_NOTIMPL;
}

STDMETHODIMP
CloakConn::ReadV(UINT32 nVecLen, UINT32* puLenVec, IHXBuffer** ppVec)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CloakConn::ReadFromV(UINT32 nVecLen, UINT32* puLenVec, IHXBuffer** ppVec, IHXSockAddr** pAddr)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CloakConn::WriteV(UINT32 nVecLen, IHXBuffer** ppVec)
{
    if (!m_pGETHandler)
    {
        return HXR_FAIL;
    }

    //XXXDPL - Should GETChannel implement WriteV() here?   
    UINT32 i = 0;
    HX_RESULT hr = HXR_OK;

    while (i < nVecLen && SUCCEEDED(hr))
    {
        hr = m_pGETHandler->SendData(ppVec[i++]);
    }

    return hr;
}

STDMETHODIMP
CloakConn::WriteToV(UINT32 nVecLen, IHXBuffer** ppVec, IHXSockAddr* pAddr)
{
    // TCP socket must be connected!
    return HXR_NOTIMPL;
}

void 
CloakConn::SetCloakMode(UINT32 ulMode)
{
    m_ulCloakMode = ulMode;
}

UINT32
CloakConn::GetCloakMode()
{
    return m_ulCloakMode;
}

HX_RESULT
CloakConn::GetGETHandler(CBaseCloakGETHandler*& pGETHandler)
{
    HX_ADDREF(m_pGETHandler);
    pGETHandler = m_pGETHandler;
    return HXR_OK;
}

HX_RESULT
CloakConn::SetGETHandler(CBaseCloakGETHandler* pGETHandler)
{
    HX_ADDREF(pGETHandler);
    HX_RELEASE(m_pGETHandler);
    m_pGETHandler = pGETHandler;
    return HXR_OK;
}

UINT32
CloakConn::GetPOSTHandlerCount()
{
    HXMutexLock(m_pPOSTHandlersLock, TRUE);
    UINT32 ulCount = m_POSTHandlers.GetCount();
    HXMutexUnlock(m_pPOSTHandlersLock);
    return ulCount;
}

HX_RESULT
CloakConn::GetFirstPOSTHandler(CBaseCloakPOSTHandler*& pPOSTHandler)
{
    HXMutexLock(m_pPOSTHandlersLock, TRUE);
    pPOSTHandler = (CBaseCloakPOSTHandler *)m_POSTHandlers.GetHead();
    if (pPOSTHandler)
    {
        pPOSTHandler->AddRef();
    }
    HXMutexUnlock(m_pPOSTHandlersLock);
    return HXR_OK;
}

HX_RESULT
CloakConn::AddPOSTHandler(CBaseCloakPOSTHandler* pPOSTHandler)
{
    HXMutexLock(m_pPOSTHandlersLock, TRUE);
    m_POSTHandlers.AddTail((void *)pPOSTHandler);
    HXMutexUnlock(m_pPOSTHandlersLock);
    return HXR_OK;
}

HX_RESULT
CloakConn::RemovePOSTHandler(CBaseCloakPOSTHandler* pPOSTHandler)
{
    HXMutexLock(m_pPOSTHandlersLock, TRUE);
    LISTPOSITION pos = m_POSTHandlers.GetHeadPosition();
    if (pos)
    {
        CBaseCloakPOSTHandler* pTmp;
        pTmp = (CBaseCloakPOSTHandler *)m_POSTHandlers.GetAt(pos);
        while (pTmp)
        {
            if (pTmp == pPOSTHandler)
            {
                m_POSTHandlers.RemoveAt(pos);
                pTmp->Release();
                HXMutexUnlock(m_pPOSTHandlersLock);
                return HXR_OK;
            }
            if (pos)
            {
                pTmp = (CBaseCloakPOSTHandler *)m_POSTHandlers.GetAtNext(pos);
            }
        }
    }
    HXMutexUnlock(m_pPOSTHandlersLock);
    return HXR_FAIL;
}

HX_RESULT
CloakConn::CloseAllPOSTHandlers()
{
    HXMutexLock(m_pPOSTHandlersLock, TRUE);
    LISTPOSITION pos = m_POSTHandlers.GetHeadPosition();
    CBaseCloakPOSTHandler* pPOSTHandler;
    if (pos)
    {
        pPOSTHandler = (CBaseCloakPOSTHandler*)m_POSTHandlers.GetAt(pos);
        pPOSTHandler->BeginClose();
    }
    while (pos)
    {
        pPOSTHandler = (CBaseCloakPOSTHandler*)m_POSTHandlers.GetAtNext(pos);
        if (pPOSTHandler)
        {
            pPOSTHandler->BeginClose();
        }
    }
    HXMutexUnlock(m_pPOSTHandlersLock);

    return HXR_OK;
}

void
CloakConn::SetRefSocket(IHXSocket* pSock)
{
    // Set reference socket for retrieving addresses, family info, etc..
    HX_ADDREF(pSock);
    IHXSocket* pOldSock = m_pRefSock;
    m_pRefSock = pSock;
    HX_RELEASE(pOldSock);
}

void 
CloakConn::OnGETRequest(CBaseCloakGETHandler* pGETHandler)
{
    SetGETHandler(pGETHandler);
    HX_ASSERT(!m_pRTSPServProt);
    RTSPServerProtocol* pNewRTSPServProt = new RTSPServerProtocol();
    pNewRTSPServProt->AddRef();
    m_pRTSPServProt = pNewRTSPServProt;
}

void 
CloakConn::GETChannelReady(Process* pProc)
{
    HX_ASSERT(m_pRTSPServProt);
    if (m_pRTSPServProt)
    {
        m_pRTSPServProt->init(pProc, this);
        m_pRTSPServProt->client()->m_bIsCloak = TRUE;
    }
}

void
CloakConn::OnGETWriteReady()
{
    HX_ASSERT(m_pRTSPServProt);
    if (m_pRTSPServProt)
    {
        //XXXDPL - We're passing events straight through to RTSP stack! 
        //Should we do event filtering here?
        m_pRTSPSvrProt->EventPending(HX_SOCK_EVENT_WRITE, HXR_OK);
    }
}

void
CloakConn::POSTChannelReady()
{
    if (m_pGETHandler)
    {
        m_pGETHandler->PostReceived();
    }
}

HX_RESULT 
CloakConn::OnPOSTData(IHXBuffer* pBuf)
{
    if (!m_pRTSPSvrProt)
    {
        return HXR_FAIL;
    }

    HX_ADDREF(pBuf);
    m_ReadQueue.AddTail((void*)pBuf);

    //XXXDPL - We're passing events straight through to RTSP stack! 
    //Should we do event filtering here?
    return m_pRTSPSvrProt->EventPending(HX_SOCK_EVENT_READ, HXR_OK);
}


