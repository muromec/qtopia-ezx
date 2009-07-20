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

#include "hxnet.h"
#include "chxrtcptransmapsocket.h"
#include "rtspserv.h"

#include "debug.h"
#include "hxheap.h"


CHXRTCPTransMapSocket::CHXRTCPTransMapSocket():CHXRTSPTranSocket()
{
    m_nRefCount = 0;
    m_uForeignPort = 0;
    m_bIsSockResponse = TRUE;
    m_bReadPending = FALSE;
    m_bEventReceived = FALSE;
    m_pPeerAddr = NULL;
    m_pFportTransResponseMap = NULL;
    m_pSockNew = NULL;
}

CHXRTCPTransMapSocket::CHXRTCPTransMapSocket(IHXSocket* pSocket):CHXRTSPTranSocket(pSocket)
    ,m_uForeignPort(0)
    ,m_bIsSockResponse(TRUE)
    ,m_bReadPending(FALSE)
    ,m_bEventReceived (FALSE)
    ,m_pPeerAddr (NULL)
    ,m_pFportTransResponseMap (NULL)
{  
    m_pSockNew = pSocket;
    HX_ADDREF(m_pSockNew);
 
}


CHXRTCPTransMapSocket::~CHXRTCPTransMapSocket(void)
{
    HX_RELEASE(m_pPeerAddr);
    HX_RELEASE(m_pSockNew);
}


STDMETHODIMP
CHXRTCPTransMapSocket::QueryInterface(REFIID riid, void** ppvObj)
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
CHXRTCPTransMapSocket::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXRTCPTransMapSocket::Release(void)
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
CHXRTCPTransMapSocket::ConnectToOne(IHXSockAddr* pAddr)
{
    return m_pSock->ConnectToOne(pAddr);
}


STDMETHODIMP
CHXRTCPTransMapSocket::ConnectToAny(UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    return m_pSock->ConnectToAny(nVecLen,ppAddrVec);
}


STDMETHODIMP
CHXRTCPTransMapSocket::SelectEvents(UINT32 uEventMask)
{
    return m_pSock->SelectEvents(uEventMask);
}


STDMETHODIMP
CHXRTCPTransMapSocket::Read(IHXBuffer** pBuf)
{
    HX_RESULT hr = HXR_FAIL;
    hr = m_pSockNew->Read(pBuf);
    m_bReadPending = FALSE;
    return hr;  
}


STDMETHODIMP
CHXRTCPTransMapSocket::PeekFrom(IHXBuffer** pBuf, IHXSockAddr** pAddr)
{
    return m_pSockNew->PeekFrom(pBuf, pAddr);
}

STDMETHODIMP
CHXRTCPTransMapSocket::ReadFrom(IHXBuffer** pBuf, IHXSockAddr** pAddr)
{
    HX_RESULT hr = HXR_FAIL;
    hr = m_pSockNew->ReadFrom(pBuf,pAddr);
    m_bReadPending = FALSE;
    return hr;
}

STDMETHODIMP
CHXRTCPTransMapSocket::Write(IHXBuffer* pBuf)
{
    
    HX_RESULT hr = HXR_FAIL;
    hr = m_pSock->WriteTo(pBuf, m_pPeerAddr);
    return hr;
}

STDMETHODIMP
CHXRTCPTransMapSocket::EventPending(UINT32 uEvent, HX_RESULT status)
{
    IHXBuffer* pbuf = NULL;
    IHXSockAddr* pAddr = NULL;
    UINT16 fport = 0;
    HX_RESULT hr = HXR_OK;

    if ( HX_SOCK_EVENT_READ == uEvent)
    {
        //if Read is pending just return, this flag is reset in Read call.
        if (!m_bReadPending)  
        {
            m_bReadPending = TRUE;
            if (m_bIsSockResponse)
            {
                if (SUCCEEDED (m_pSock->PeekFrom(&pbuf, &pAddr))) //peek does not empty the socket queue.
                {   
                    if (pAddr != NULL)
                    {
                        fport = pAddr->GetPort();
                    }       
                }
                else
                {
                    HX_RELEASE(pbuf);
                    HX_RELEASE(pAddr);
                    
                    return HXR_FAIL;
                }
                HX_RELEASE(pbuf);
                HX_RELEASE(pAddr);

                if (fport != NULL && fport != m_uForeignPort)
                {
                    //find the correct Transport Response.
                    CHXRTCPTransMapSocket* pTransSocket = NULL;
                    HX_ASSERT(m_pFportTransResponseMap);
                    if ( m_pFportTransResponseMap->Lookup(fport, (void*&)pTransSocket))
                    {
                        pTransSocket->AddRef();
                        pTransSocket->SetSocket(this);
                        hr = pTransSocket->EventPending(uEvent, status);  
                        HX_RELEASE(pTransSocket);
                    }
                    else
                    {
                        //No Transport found!! Just discard the buffer.
                        Read(&pbuf);
                        HX_RELEASE(pbuf);
                    }
                    
                }
                else
                {
                    //No mapping needed.
                    hr = m_pResponse->EventPending(uEvent,status);
                }
            }
            else
            {
            //TransResponse 
            hr = m_pResponse->EventPending(uEvent, status);
            }
        }           
        HX_RELEASE(pbuf);
        HX_RELEASE(pAddr);
        m_bEventReceived = TRUE;
    }

    return hr;
}

void
CHXRTCPTransMapSocket::SetSocket(IHXSocket* pSock)
{
    if (m_pSockNew == pSock)
    {
        return;
    }

    HX_RELEASE(m_pSockNew);   
    HX_ASSERT(pSock);
    m_pSockNew = pSock;
    HX_ADDREF(m_pSockNew);

    if (m_pSock == NULL)
    {
        m_pSock = pSock;
        HX_ADDREF(m_pSock);
    }
}


IHXSocket*
CHXRTCPTransMapSocket::GetSocket(void)
{
    return m_pSockNew;
}

void
CHXRTCPTransMapSocket::SetIsSockResponse(BOOL b)
{
    m_bIsSockResponse = b;
}

 
void
CHXRTCPTransMapSocket::SetSocketPort(IHXSocket* pSocket,UINT16 fport)
{
    HX_ASSERT(pSocket);
    HX_RELEASE(m_pSock);
    m_pSock = pSocket;
    HX_ADDREF(m_pSock);

    HX_RELEASE(m_pSockNew);
    m_pSockNew = pSocket;
    HX_ADDREF(m_pSockNew);
        
    m_uForeignPort = fport;
}

void 
CHXRTCPTransMapSocket::SetPeerAddr(IHXSockAddr* pPeerAddr)
{
    HX_ASSERT(pPeerAddr && !m_pPeerAddr); //This should be set only once.
    m_pPeerAddr = pPeerAddr;
    HX_ADDREF(m_pPeerAddr);
}

void 
CHXRTCPTransMapSocket::SetMap(CHXMapLongToObj* pFportTransResponseMap)
{
    HX_ASSERT(pFportTransResponseMap);
    //Session owns this map pointer, and gets deleted when the session is destroyed.
    //This socket gets closed with Session Done().
    //So the pointer will be available for the life of this socket.
    m_pFportTransResponseMap = pFportTransResponseMap;
}

STDMETHODIMP
CHXRTCPTransMapSocket::Close(void)
{
     HX_RELEASE(m_pResponse);

    // we dont want call Close() twice on same socket.
    if ( m_pSockNew == m_pSock)
    {
        if (m_pSockNew != NULL)
        {
            m_pSockNew->Close();
        }
        HX_RELEASE(m_pSockNew);
        HX_RELEASE(m_pSock);
    }
    else
    {
        if (m_pSockNew != NULL)
        {
            m_pSockNew->Close();
        }
        HX_RELEASE(m_pSockNew);

        if (m_pSock != NULL)
        {
            m_pSock->Close();
        }
        HX_RELEASE(m_pSock);
    }
    
    return HXR_OK;
}