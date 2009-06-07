/* ***** BEGIN LICENSE BLOCK *****
 *
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
#include "hxcom.h"
#include "hxnet.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "hxcbobj.h"
#include "hxtick.h"
#include "netbyte.h"
#include "hxsockutil.h"
#include "shim_utils.h"
#include "shim_socket.h"
#define HELIX_FEATURE_LOGLEVEL_NONE
#include "hxtlogutil.h"

#if defined(_SYMBIAN) || defined(_SOLARIS)
#include <sys/types.h>
#include <sys/socket.h>
#endif /* _SYMBIAN */

#include "hxassert.h"
#define HX_SOCKET_SHIM_TCPREADSIZE  4096
#define HX_SOCKET_SHIM_UDPREADSIZE 65535

CHXClientSocketShim::CHXClientSocketShim(IUnknown* pContext, 
                                         IHXNetServices* pNetServices, 
                                         IHXNetworkServices* pOldNetServices)
{
    InitSocket(pContext, pNetServices, pOldNetServices);
}

CHXClientSocketShim::~CHXClientSocketShim()
{
    Close();
}


STDMETHODIMP CHXClientSocketShim::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT retVal = HXR_NOINTERFACE;

    if (ppvObj)
    {
        // Set default
        *ppvObj = NULL;
        // Switch on riid
        if (IsEqualIID(riid, IID_IUnknown))
        {
            AddRef();
            *ppvObj = (IHXSocket*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXSocket))
        {
            AddRef();
            *ppvObj = (IHXSocket*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXMulticastSocket))
        {
            AddRef();
            *ppvObj = (IHXMulticastSocket*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXTCPResponse))
        {
            AddRef();
            *ppvObj = (IHXTCPResponse*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXUDPResponse))
        {
            AddRef();
            *ppvObj = (IHXUDPResponse*) this;
            retVal  = HXR_OK;
        }
    }

    return retVal;
}


STDMETHODIMP
CHXClientSocketShim::JoinGroup(IHXSockAddr* pGroupAddr,
                                           IHXSockAddr* pInterface)
{
    HX_RESULT hr = HXR_FAIL;
    if (m_pUDPSocket)
    {
        UINT32 ulAddrGroup = 0;
        HXSockUtil::GetIN4Address(pGroupAddr, ulAddrGroup);

        UINT32 ulAddrIface = 0;
        if (pInterface)
        {
            HXSockUtil::GetIN4Address(pInterface, ulAddrIface);
        }

        hr = m_pUDPSocket->JoinMulticastGroup(ulAddrGroup, ulAddrIface);
    }
    return hr;
}

STDMETHODIMP
CHXClientSocketShim::LeaveGroup(IHXSockAddr* pGroupAddr,
                                           IHXSockAddr* pInterface)
{
    HX_RESULT hr = HXR_FAIL;
    if (m_pUDPSocket)
    {
        UINT32 ulAddrGroup = 0;
        HXSockUtil::GetIN4Address(pGroupAddr, ulAddrGroup);

        UINT32 ulAddrIface = 0;
        if (pInterface)
        {
            HXSockUtil::GetIN4Address(pInterface, ulAddrIface);
        }

        hr = m_pUDPSocket->LeaveMulticastGroup(ulAddrGroup, ulAddrIface);
    }
    return hr;
}


STDMETHODIMP CHXClientSocketShim::SetSourceOption(HXMulticastSourceOption flag,
                                       IHXSockAddr* pSourceAddr,
                                       IHXSockAddr* pGroupAddr,
                                       IHXSockAddr* pInterface)
{
    HX_ASSERT(FALSE);
    return HXR_NOTIMPL;
}

STDMETHODIMP_(ULONG32) CHXClientSocketShim::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXClientSocketShim::Release(void)
{
    INT32 rc = InterlockedDecrement(&m_lRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return (ULONG32) rc;
}

STDMETHODIMP CHXClientSocketShim::SetResponse(IHXSocketResponse* pResponse)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pResponse)
    {
        // Save the socket response interface
        HX_RELEASE(m_pResponse);
        m_pResponse = pResponse;
        m_pResponse->AddRef();
        // Clear the return value
        retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP CHXClientSocketShim::SetAccessControl(IHXSocketAccessControl* pControl)
{
    return HXR_OK;
}

STDMETHODIMP CHXClientSocketShim::Init(HXSockFamily f, HXSockType t, HXSockProtocol p)
{
    HX_RESULT retVal = HXR_FAIL;

    // Filter out families, types, and protocols
    // that the shim can't support
    if ((f == HX_SOCK_FAMILY_IN4 || f == HX_SOCK_FAMILY_INANY) &&
        (t == HX_SOCK_TYPE_UDP   || t == HX_SOCK_TYPE_TCP)     &&
         p == HX_SOCK_PROTO_ANY)
    {
        // Save the family, type, and protocol
        m_eSockFamily   = HX_SOCK_FAMILY_IN4;
        m_eSockType     = t;
        m_eSockProtocol = p;
        
        if (t == HX_SOCK_TYPE_TCP)
        {
            // Create a IHXTCPSocket
            HX_RELEASE(m_pTCPSocket);
            retVal = m_pOldNetServices->CreateTCPSocket(&m_pTCPSocket);
            if (SUCCEEDED(retVal))
            {
                // Get our IHXTCPResponse interface
                IHXTCPResponse* pResp = NULL;
                retVal = QueryInterface(IID_IHXTCPResponse, (void**) &pResp);
                if (SUCCEEDED(retVal))
                {
                    // Init the IHXTCPSocket
                    retVal = m_pTCPSocket->Init(pResp);
                    if (SUCCEEDED(retVal))
                    {
                        // Set the flag saying we've initialized the socket
                        m_eSocketState = SocketStateOpen;
                    }
                }
                HX_RELEASE(pResp);
            }
        }
        else if (t == HX_SOCK_TYPE_UDP)
        {
            HX_RELEASE(m_pUDPSocket);
            retVal = m_pOldNetServices->CreateUDPSocket(&m_pUDPSocket);
            if (SUCCEEDED(retVal))
            {
                // Get our IHXUDPResponse interface
                IHXUDPResponse* pResp = NULL;
                retVal = QueryInterface(IID_IHXUDPResponse, (void**) &pResp);
                if (SUCCEEDED(retVal))
                {
                    // Init the IHXUDPSocket
                    retVal = m_pUDPSocket->Init(0, 0, pResp);
                    if (SUCCEEDED(retVal))
                    {
                        // Set the flag saying we've initialized the socket
                        m_eSocketState = SocketStateOpen;
                    }
                }
                HX_RELEASE(pResp);
            }
        }

    }
    return retVal;
}

STDMETHODIMP CHXClientSocketShim::CreateSockAddr(IHXSockAddr** ppAddr)
{
    HX_RESULT retVal = HXR_FAIL;
    
    if (ppAddr && m_pNetServices)
    {
        retVal = m_pNetServices->CreateSockAddr(m_eSockFamily, ppAddr);
    }

    return retVal;
}

STDMETHODIMP CHXClientSocketShim::Bind(IHXSockAddr* pAddr)
{
    HX_RESULT retVal = HXR_FAIL;
    HX_ASSERT(pAddr && pAddr->GetFamily() == HX_SOCK_FAMILY_IN4);

    // Filter out everything but IPv4 socket families
    if (pAddr && pAddr->GetFamily() == HX_SOCK_FAMILY_IN4 &&
        m_eSocketState == SocketStateOpen)
    {
        // Get the IN4 address (native order)
        UINT32 ulAddr = 0;
        retVal = HXSockUtil::GetIN4Address(pAddr, ulAddr);
        if (SUCCEEDED(retVal))
        {
            // Get the IN4 port (native order)
            UINT16 usPort = 0;
            retVal = HXSockUtil::GetIN4Port(pAddr, usPort);
            if (SUCCEEDED(retVal))
            {
                if (m_pTCPSocket)
                {
                    retVal = m_pTCPSocket->Bind(ulAddr, usPort);
                    if (SUCCEEDED(retVal))
                    {
                        m_eSocketState = SocketStateNamed;
                    }
                }
                else if (m_pUDPSocket)
                {
                    retVal = m_pUDPSocket->Bind(ulAddr, usPort);
                    if (SUCCEEDED(retVal))
                    {
                        m_eSocketState = SocketStateNamed;
                    }
                }
            }
        }
    }

    return retVal;
}

STDMETHODIMP CHXClientSocketShim::ConnectToOne(IHXSockAddr* pAddr)
{
    // We define a separate method to implement ConnectToOne()
    // since the code is shared with ConnectToAny().
    return DoConnectToOne(pAddr);

}

STDMETHODIMP CHXClientSocketShim::ConnectToAny(UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    HX_RESULT retVal = HXR_FAIL;

    HX_ASSERT(!m_pUDPSocket);
    if (nVecLen && ppAddrVec)
    {
        // Copy the address vector
        ClearAddressVector();
        m_ppSockAddr = new IHXSockAddr* [nVecLen];
        if (m_ppSockAddr)
        {
            // Copy all the addresses
            memcpy(m_ppSockAddr, ppAddrVec, nVecLen * sizeof(IHXSockAddr*));
            // AddRef all the addresses
            for (UINT32 i = 0; i < nVecLen; i++)
            {
                if (m_ppSockAddr[i])
                {
                    m_ppSockAddr[i]->AddRef();
                }
            }
            // Save the number of addresses
            m_ulNumSockAddr = nVecLen;
            // Initialize the current address
            m_ulCurSockAddr = 0;
            // Try connecting to the first address
            retVal = DoConnectToOne(m_ppSockAddr[m_ulCurSockAddr]);
        }
    }

    return retVal;
}

STDMETHODIMP CHXClientSocketShim::GetLocalAddr(IHXSockAddr** ppAddr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (ppAddr)
    {
        UINT32 ulAddr = 0;
        UINT16 usPort = 0;
        if (m_pTCPSocket)
        {
            // Get local address (native order)
            retVal = m_pTCPSocket->GetLocalAddress(ulAddr);
            if (SUCCEEDED(retVal))
            {
                // Get local port (native order)
                retVal = m_pTCPSocket->GetLocalPort(usPort);
                if (SUCCEEDED(retVal))
                {
                    IHXSockAddr* pAddr = NULL;
                    retVal = HXSockUtil::CreateAddrIN4(m_pNetServices, ulAddr, usPort, pAddr);
                    if (SUCCEEDED(retVal))
                    {
                        pAddr->AddRef();
                        *ppAddr = pAddr;
                    }
                    HX_RELEASE(pAddr);
                }
            }
        }
        else if (m_pUDPSocket)
        {
            // Assume a local address
            UINT32 ulAddr = 0;
            // Get local port (native order)
            retVal = m_pUDPSocket->GetLocalPort(usPort);
            if (SUCCEEDED(retVal))
            {
                IHXSockAddr* pAddr = NULL;
                retVal = HXSockUtil::CreateAddrIN4(m_pNetServices, ulAddr, usPort, pAddr);
                if (SUCCEEDED(retVal))
                {
                    pAddr->AddRef();
                    *ppAddr = pAddr;
                }
                HX_RELEASE(pAddr);
            }
        }
    }

    return retVal;
}

STDMETHODIMP CHXClientSocketShim::GetPeerAddr(IHXSockAddr** ppAddr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (ppAddr)
    {
        UINT32 ulAddr = 0;
        UINT16 usPort = 0;
        if (m_pTCPSocket)
        {
            // Get foreign address (native order)
            retVal = m_pTCPSocket->GetForeignAddress(ulAddr);
            if (SUCCEEDED(retVal))
            {
                // Get foreign port (native order)
                retVal = m_pTCPSocket->GetForeignPort(usPort);
                if (SUCCEEDED(retVal))
                {
                    IHXSockAddr* pAddr = NULL;
                    retVal = HXSockUtil::CreateAddrIN4(m_pNetServices, ulAddr, usPort, pAddr);
                    if (SUCCEEDED(retVal))
                    {
                        pAddr->AddRef();
                        *ppAddr = pAddr;
                    }
                    HX_RELEASE(pAddr);
                }
            }
        }
        else if (m_pUDPSocket && m_pUDPConnectAddr)
        {
            // Return address passed to ConnectTo()
            *ppAddr = m_pUDPConnectAddr;
            (*ppAddr)->AddRef();
            retVal = HXR_OK;
        }
    
    }

    return retVal;
}

STDMETHODIMP CHXClientSocketShim::SelectEvents(UINT32 uEventMask)
{
    m_ulEventMask = uEventMask;

    if (m_pUDPSocket)
    {
        if (uEventMask & HX_SOCK_EVENT_READ)
        {
            // socket is now writable; schedule an underlying read immediately
            ScheduleCallback(HX_SOCK_EVENT_WRITE |
                             HX_SOCKET_SHIM_FLAG_ISSUEREAD);
        }
    }

    return HXR_OK;
}

STDMETHODIMP CHXClientSocketShim::Peek(IHXBuffer** ppBuf)
{
    return DoRead(ppBuf, TRUE);
}

STDMETHODIMP CHXClientSocketShim::Read(IHXBuffer** ppBuf)
{
    return DoRead(ppBuf, FALSE);
}

STDMETHODIMP CHXClientSocketShim::Write(IHXBuffer* pBuf)
{
    HX_RESULT retVal = HXR_FAIL;

    // Sanity check
    if (pBuf)
    {
        // Write to the IHXTCPSocket or the IHXUDPSocket
        if (m_pTCPSocket)
        {
            retVal = m_pTCPSocket->Write(pBuf);
        }
        else if (m_pUDPSocket)
        {
            // ConnectTo() must be called with default peer addr for this to work with UDP
            HX_ASSERT(m_pUDPConnectAddr);
            HX_ASSERT(SocketStateConnected == m_eSocketState);
            if (m_pUDPConnectAddr)
            {
                retVal = WriteTo(pBuf, m_pUDPConnectAddr);
                HX_ASSERT(SUCCEEDED(retVal));
            }
        }
        if (SUCCEEDED(retVal))
        {
            // Since writing re-enables the write event, then
            // we will schedule a callback to issue a write event
            ScheduleCallback(HX_SOCK_EVENT_WRITE);
        }
    }

    return retVal;
}

STDMETHODIMP CHXClientSocketShim::Close()
{
    if (m_pSocketCallback &&
        m_pSocketCallback->IsCallbackPending())
    {
        m_pSocketCallback->Cancel(m_pScheduler);
    }
    HX_RELEASE(m_pSocketCallback);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pTCPSocket);
    HX_RELEASE(m_pUDPSocket);
    HX_RELEASE(m_pLastReadBuffer);
    HX_RELEASE(m_pOldNetServices);
    HX_RELEASE(m_pNetServices);
    HX_RELEASE(m_pUDPConnectAddr);
    ClearAddressVector();

    return HXR_OK;
}

STDMETHODIMP CHXClientSocketShim::Listen(UINT32 uBackLog)
{
    // In the new netservices implementation, the
    // IHXListeningSocket uses an underlying IHXSocket to
    // implement Listen(). However, for the shim implementation
    // of IHXListeningSocket, we use a old-style 
    // IHXListenSocket. Therefore we don't need to 
    // implement CHXClientSocketShim::Listen().
    return HXR_NOTIMPL;
}

STDMETHODIMP CHXClientSocketShim::Accept(IHXSocket** ppNewSock, IHXSockAddr** ppSource)
{
    // In the new netservices implementation, the
    // IHXListeningSocket uses an underlying IHXSocket to
    // implement Accep(). However, for the shim implementation
    // of IHXListeningSocket, we use a old-style 
    // IHXListenSocket. Therefore we don't need to 
    // implement CHXClientSocketShim::Accept().
    return HXR_NOTIMPL;
}

STDMETHODIMP CHXClientSocketShim::GetOption(HXSockOpt name, UINT32* pval)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CHXClientSocketShim::SetOption(HXSockOpt name, UINT32 val)
{
    HX_RESULT retVal = HXR_FAIL;

    IHXSetSocketOption* pSetSocketOption = NULL;
    if (m_pTCPSocket)
    {
        retVal = m_pTCPSocket->QueryInterface(IID_IHXSetSocketOption, (void**) &pSetSocketOption);
    }
    else if (m_pUDPSocket)
    {
        retVal = m_pUDPSocket->QueryInterface(IID_IHXSetSocketOption, (void**) &pSetSocketOption);
    }
    if (SUCCEEDED(retVal))
    {
        // Translate the option
        HX_SOCKET_OPTION eOpt = HX_SOCKOPT_REUSE_ADDR;
        retVal = CHXShimUtils::TranslateFromHXSockOpt(name, eOpt);
        if (SUCCEEDED(retVal))
        {
            // Set the option
            retVal = pSetSocketOption->SetOption(eOpt, val);
        }
    }
    HX_RELEASE(pSetSocketOption);

    return retVal;
}


STDMETHODIMP CHXClientSocketShim::PeekFrom(IHXBuffer** ppBuf, IHXSockAddr** ppAddr)
{
    return DoReadFrom(ppBuf, ppAddr, TRUE);
}

STDMETHODIMP CHXClientSocketShim::ReadFrom(IHXBuffer** ppBuf, IHXSockAddr** ppAddr)
{
    return DoReadFrom(ppBuf, ppAddr, FALSE);
}

STDMETHODIMP CHXClientSocketShim::WriteTo(IHXBuffer* pBuf, IHXSockAddr* pAddr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pBuf && pAddr)
    {
        // Get the address (native order)
        UINT32 ulAddr = 0;
        retVal = HXSockUtil::GetIN4Address(pAddr, ulAddr);
        if (SUCCEEDED(retVal))
        {
            // Get the port (native order)
            UINT16 usPort = 0;
            retVal = HXSockUtil::GetIN4Port(pAddr, usPort);
            if (SUCCEEDED(retVal))
            {
                // Now do the write
                if (m_pTCPSocket)
                {
                    // We don't really expect to get a WriteTo() on
                    // a TCP Socket, since it's already connected. So if
                    // we do, we will just treat this as a Write()
                    retVal = m_pTCPSocket->Write(pBuf);
                }
                else if (m_pUDPSocket)
                {
                    // Write to the socket
                    retVal = m_pUDPSocket->WriteTo(ulAddr, usPort, pBuf);
                    HX_ASSERT(SUCCEEDED(retVal));
                }
                else
                {
                    retVal = HXR_UNEXPECTED;
                }
                if (SUCCEEDED(retVal))
                {
                    // Since writing re-enables the write event, then
                    // we will schedule a callback to issue a write event
                    ScheduleCallback(HX_SOCK_EVENT_WRITE);
                }
            }
        }
    }

    return retVal;
}

STDMETHODIMP CHXClientSocketShim::ReadV(UINT32 nVecLen, UINT32* puLenVec, IHXBuffer** ppBufVec)
{
    // XXXMEH - haven't implemented vectored I/O yet
    return HXR_NOTIMPL;
}

STDMETHODIMP CHXClientSocketShim::ReadFromV(UINT32 nVecLen, UINT32* puLenVec,
                                            IHXBuffer** ppBufVec, IHXSockAddr** ppAddr)
{
    // XXXMEH - haven't implemented vectored I/O yet
    return HXR_NOTIMPL;
}

STDMETHODIMP CHXClientSocketShim::WriteV(UINT32 nVecLen, IHXBuffer** ppBufVec)
{
    // XXXMEH - haven't implemented vectored I/O yet
    return HXR_NOTIMPL;
}

STDMETHODIMP CHXClientSocketShim::WriteToV(UINT32 nVecLen, IHXBuffer** ppBufVec, IHXSockAddr* pAddr)
{
    // XXXMEH - haven't implemented vectored I/O yet
    return HXR_NOTIMPL;
}

STDMETHODIMP CHXClientSocketShim::ConnectDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    HX_ASSERT(m_eSocketState == SocketStateConnectionPending);
    if (m_eSocketState == SocketStateConnectionPending)
    {
        // Clear the return value
        retVal = HXR_OK;
        // Clear the flag saying we want to 
        // send the events
        // Were we successful?
        if (SUCCEEDED(status))
        {
            // Set the state
            m_eSocketState = SocketStateConnected;
            // If we were doing a ConnectToAny(), we don't
            // need the address vector anymore.
            ClearAddressVector();
            // Schedule a connect event, a write event
            // and a socket read
            ScheduleCallback(HX_SOCK_EVENT_CONNECT |
                             HX_SOCK_EVENT_WRITE   |
                             HX_SOCKET_SHIM_FLAG_ISSUEREAD);
        }
        else
        {
            // We failed to connect.
            //
            // Were we doing a ConnectToAny and have some
            // addresses left to try?
            if (m_ppSockAddr &&
                m_ulCurSockAddr + 1 < m_ulNumSockAddr &&
                m_ppSockAddr[m_ulCurSockAddr + 1])
            {
                // Leave the state as ConnectionPending
                //
                // Increment the current socket index
                m_ulCurSockAddr++;
                // We have another address to try, so 
                // don't send any events yet (leave the
                // flags FALSE) and try the next address
                retVal = DoConnectToOne(m_ppSockAddr[m_ulCurSockAddr]);
            }
            else
            {
                // We either were not doing a ConnectToAny, or
                // we were doing a ConnectToAny and we don't
                // have any more addresses to try. So send the
                // connect event (so that the user will be able
                // to see the associated HXR_SOCK_CONNREFUSED
                // error), but don't send the write event


                // Set the state back to SocketStateOpen
                m_eSocketState = SocketStateOpen;
                // We still need to issue a Connect event
                // so that the user can then get the error 
                // associated with the connect.
                m_pResponse->EventPending(HX_SOCK_EVENT_CONNECT, HXR_SOCK_CONNREFUSED);
                //ScheduleCallback(HX_SOCK_EVENT_CONNECT);
            }
        }
    }

    return retVal;
}

STDMETHODIMP CHXClientSocketShim::ReadDone(HX_RESULT status, IHXBuffer* pBuffer)
{
    HX_RESULT retVal = HXR_FAIL;
    m_bReadDonePending = FALSE;

    if (SUCCEEDED(status))
    {
        // Check again to make sure that the user
        // still has HX_SOCK_EVENT_READ selected.
        HX_ASSERT(m_ulEventMask & HX_SOCK_EVENT_READ);
        if (m_ulEventMask & HX_SOCK_EVENT_READ)
        {
            // Clear the return value
            retVal = HXR_OK;
            // Save the read buffer
            HX_ASSERT(!m_pLastReadBuffer); // if this buffer exists, we are LOSING IT!
            HX_RELEASE(m_pLastReadBuffer);
            m_pLastReadBuffer = pBuffer;
            m_pLastReadBuffer->AddRef();
            // Init the buffer offset
            m_ulLastReadOffset = 0;
            // Schedule a callback to send the HX_SOCK_EVENT_READ event
            ScheduleCallback(HX_SOCK_EVENT_READ);
        }
    }
    else
    {
        // The read failed, so issue a close event
        ScheduleCallback(HX_SOCK_EVENT_CLOSE);
    }

    return retVal;
}

STDMETHODIMP CHXClientSocketShim::WriteReady(HX_RESULT status)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CHXClientSocketShim::Closed(HX_RESULT status)
{
    HX_RESULT retVal = HXR_OK;

    // We need to schedule a close event callback
    if (m_ulEventMask & HX_SOCK_EVENT_CLOSE)
    {
        ScheduleCallback(HX_SOCK_EVENT_CLOSE);
    }

    return retVal;
}

STDMETHODIMP CHXClientSocketShim::ReadDone(HX_RESULT status, IHXBuffer* pBuffer, ULONG32 ulAddr, UINT16 nPort)
{
    HXLOGL4(HXLOG_NETW, "%p::CHXClientSocketShim::ReadDone(status=0x%08x,pBuffer=%p,ulAddr=%lu,nPort=%u)", this, status, pBuffer, ulAddr, nPort);
    
    HX_RESULT retVal = HXR_FAIL;
    m_bReadDonePending = FALSE;

    if (SUCCEEDED(status))
    {
        // Check again to make sure that the user
        // still has HX_SOCK_EVENT_READ selected.
        if (m_ulEventMask & HX_SOCK_EVENT_READ)
        {
            // Clear the return value
            retVal = HXR_OK;
            // Save the read buffer
            HX_ASSERT(!m_pLastReadBuffer); // if this buffer exists, we are LOSING IT!
            HX_RELEASE(m_pLastReadBuffer);
            m_pLastReadBuffer = pBuffer;
            m_pLastReadBuffer->AddRef();

            // Save the UDP read address and port
            m_ulUDPReadAddress = ulAddr;
            m_usUDPReadPort    = nPort;
            
            // Init the buffer offset
            m_ulLastReadOffset = 0;
            // Schedule a callback to send the HX_SOCK_EVENT_READ event
            ScheduleCallback(HX_SOCK_EVENT_READ);
        }
    }

    return retVal;
}

HX_RESULT CHXClientSocketShim::Init(IHXTCPSocket* pSocket)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pSocket)
    {
        // Init the family, type, and protocol
        m_eSockFamily   = HX_SOCK_FAMILY_IN4;
        m_eSockType     = HX_SOCK_TYPE_TCP;
        m_eSockProtocol = HX_SOCK_PROTO_ANY;
        // Save the socket
        HX_RELEASE(m_pTCPSocket);
        m_pTCPSocket = pSocket;
        m_pTCPSocket->AddRef();
        // Get our IHXTCPResponse interface
        IHXTCPResponse* pResp = NULL;
        retVal = QueryInterface(IID_IHXTCPResponse, (void**) &pResp);
        if (SUCCEEDED(retVal))
        {
            // Init the IHXTCPSocket
            retVal = m_pTCPSocket->Init(pResp);
            if (SUCCEEDED(retVal))
            {
                // Set the flag saying we've initialized the socket
                m_eSocketState = SocketStateOpen;
            }
        }
        HX_RELEASE(pResp);
    }

    return retVal;
}

HX_RESULT CHXClientSocketShim::ScheduleCallback(UINT32 ulTask)
{
    HX_RESULT retVal = HXR_FAIL;

    if (ulTask)
    {
        // Allocate a callback if we do not already have one
        if (!m_pSocketCallback)
        {
            m_pSocketCallback = new CHXGenericCallback((void*) this, SocketCallbackProc);
            if (m_pSocketCallback)
            {
                m_pSocketCallback->AddRef();
            }
        }
        if (m_pSocketCallback)
        {
            // Clear the return value
            retVal = HXR_OK;
            // We want to set the flag in the saved socket
            // flags if the bit is set either in ulTask
            // or in m_ulSocketTaskFlags
            m_ulSocketTaskFlags |= ulTask;
            // Is a callback already scheduled?
            if (!m_pSocketCallback->IsCallbackPending())
            {
                // We don't have a callback scheduled, so schedule one
                m_pSocketCallback->ScheduleRelative(m_pScheduler, 0);
            }
        }
    }

    return retVal;
}

void CHXClientSocketShim::HandleCallback()
{
    if (m_ulSocketTaskFlags)
    {
        // Make a local copy of the socket task flags.
        // This is because calls to EventPending() can
        // cause ScheduleCallback() to be called and
        // can cause events to be fired from *this*
        // callback when they are intended to be called
        // from the *next* callback.
        UINT32 ulLocalTaskFlags = m_ulSocketTaskFlags;
        // Clear the socket task flags
        m_ulSocketTaskFlags = 0;
        // HX_SOCK_EVENT_xxx events are defined from (1<<1),
        // (1<<2),...,HX_SOCK_EVENT_LAST. We iterate through
        // then in reverse because if we have a connect and
        // a write event in the same callback, we want the
        // connect event to be fired before the write.
        if (m_pResponse)
        {
            for (UINT32 i = HX_SOCK_EVENT_LAST; i > 0; i >>= 1)
            {
                // The bit has to be set both in the
                // event task flag and the event mask
                if (ulLocalTaskFlags & m_ulEventMask & i)
                {
                    // Clear the socket task flag
                    ulLocalTaskFlags &= ~i;
                    // Call the event
                    m_pResponse->EventPending(i, HXR_OK); //XXXLCM need error code
                }
            }
        }
        // Are we supposed to issue a socket read?
        // if m_pLastReadBuffer is non-null it means that the response
        // did not read in response to EventPending yet, and we must
        // not do more reading from the tcp or udp socket or we would
        // lose data.
        if (!m_pLastReadBuffer && (ulLocalTaskFlags & HX_SOCKET_SHIM_FLAG_ISSUEREAD))
        {
            // Clear the socket task flag
            ulLocalTaskFlags &= ~HX_SOCKET_SHIM_FLAG_ISSUEREAD;
            if (!m_bReadDonePending)
            {
                m_bReadDonePending = TRUE;

                // Issue the read
                if (m_pTCPSocket)
                {
                    m_pTCPSocket->Read(HX_SOCKET_SHIM_TCPREADSIZE);
                }
                else if (m_pUDPSocket)
                {
                    m_pUDPSocket->Read(HX_SOCKET_SHIM_UDPREADSIZE);
                }
            }
        }
    }
}

void CHXClientSocketShim::ClearAddressVector()
{
    if (m_ulNumSockAddr && m_ppSockAddr)
    {
        for (UINT32 i = 0; i < m_ulNumSockAddr; i++)
        {
            HX_RELEASE(m_ppSockAddr[i]);
        }
    }
    HX_VECTOR_DELETE(m_ppSockAddr);
}

HX_RESULT CHXClientSocketShim::DoConnectToOne(IHXSockAddr* pAddr)
{
    if (!pAddr || pAddr->GetFamily() != HX_SOCK_FAMILY_IN4)
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }
        

    HX_RESULT retVal = HXR_FAIL;

    HX_ASSERT(m_eSocketState == SocketStateOpen || m_eSocketState == SocketStateNamed);
    if (m_eSocketState == SocketStateOpen || m_eSocketState == SocketStateNamed)
    {
        IHXBuffer* pAddrString = NULL;
        retVal = pAddr->GetAddr(&pAddrString);
        if (SUCCEEDED(retVal))
        {
            if (m_pTCPSocket)
            {
                // Get the port (native order)
                UINT16 usPort = 0;
                retVal = HXSockUtil::GetIN4Port(pAddr, usPort);
                if (SUCCEEDED(retVal))
                {
                    // Set the state
                    m_eSocketState = SocketStateConnectionPending;
                    // Call Connect
                    retVal = m_pTCPSocket->Connect((const char*)pAddrString->GetBuffer(), usPort);
                }
            }
            else if (m_pUDPSocket)
            {
                // UDP Sockets are connectionless, so really we
                // don't expect to get called with ConnectToOne()
                // or ConnectToAny() for a UDP socket. If we do,
                // however, then we will simply go ahead and pretend
                // that the connection succeeded and schedule a
                // callback as if the connection succeeded.
                //
                
                // Set the state
                m_eSocketState = SocketStateConnected;

                HX_RELEASE(m_pUDPConnectAddr);
                m_pUDPConnectAddr = pAddr;
                m_pUDPConnectAddr->AddRef();
                // If we were doing a ConnectToAny(), we don't
                // need the address vector anymore.
                ClearAddressVector();
                // Schedule a connect event, a write event
                // and a socket read
                ScheduleCallback(HX_SOCK_EVENT_CONNECT |
                                 HX_SOCK_EVENT_WRITE   |
                                 HX_SOCKET_SHIM_FLAG_ISSUEREAD);
            }
            else
            {
                retVal = HXR_FAIL;
            }
            HX_RELEASE(pAddrString);
        }
    }

    return retVal;
}

void CHXClientSocketShim::InitSocket(IUnknown* pContext, 
                                     IHXNetServices* pNetServices, 
                                     IHXNetworkServices* pOldNetServices)
{
    HX_ASSERT(pContext);
    HX_ASSERT(pNetServices);
    HX_ASSERT(pOldNetServices);

    m_lRefCount         = 0;
    m_pContext          = pContext;
    m_eSockFamily       = HX_SOCK_FAMILY_NONE;
    m_eSockType         = HX_SOCK_TYPE_NONE;
    m_eSockProtocol     = HX_SOCK_PROTO_NONE;
    m_pNetServices      = pNetServices;
    m_pOldNetServices   = pOldNetServices;
    m_pResponse         = NULL;
    m_pScheduler        = NULL;
    m_pTCPSocket        = NULL;
    m_pUDPSocket        = NULL;
    m_ulEventMask       = 0;
    m_ulNumSockAddr     = 0;
    m_ulCurSockAddr     = 0;
    m_ppSockAddr        = NULL;
    m_eSocketState      = SocketStateClosed;
    m_pLastReadBuffer   = NULL;
    m_ulLastReadOffset  = 0;
    m_pSocketCallback   = NULL;
    m_ulSocketTaskFlags = 0;
    m_ulUDPReadAddress  = 0;
    m_usUDPReadPort     = 0;
    m_bBigEndian        = TestBigEndian();
    m_bReadDonePending  = FALSE;
    m_pUDPConnectAddr   = NULL;

    m_pContext->AddRef();
    m_pNetServices->AddRef();
    m_pOldNetServices->AddRef();

    // QI for IHXScheduler
    m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
    
}

HX_RESULT CHXClientSocketShim::DoRead(IHXBuffer** ppBuffer, HXBOOL bPeek)
{
    HX_RESULT retVal = HXR_SOCK_WOULDBLOCK;

    if (ppBuffer)
    {
        if (m_pLastReadBuffer)
        {
            // Clear the return value
            retVal = HXR_OK;
            // AddRef the last read buffer before going out
            m_pLastReadBuffer->AddRef();
            // Copy the output buffer
            HX_RELEASE(*ppBuffer);
            *ppBuffer = m_pLastReadBuffer;
            // Are we peeking or reading?
            if (!bPeek)
            {
                // We are reading, so we can release
                // the ref on the buffer
                HX_RELEASE(m_pLastReadBuffer);
                // Schedule a callback for another IHXTCPSocket Read
                ScheduleCallback(HX_SOCKET_SHIM_FLAG_ISSUEREAD);
            }
        }
        else
        {
            // no data to read right now; ensure underlying read is scheduled
            ScheduleCallback(HX_SOCKET_SHIM_FLAG_ISSUEREAD);
        }
    }
    else
    {
        HX_ASSERT(FALSE);
        retVal = HXR_INVALID_PARAMETER;
    }

    return retVal;
}

HX_RESULT CHXClientSocketShim::DoReadFrom(IHXBuffer** ppBuffer, IHXSockAddr** ppAddr, HXBOOL bPeek)
{
    HX_RESULT retVal = HXR_SOCK_WOULDBLOCK;

    if (ppBuffer && ppAddr)
    {
        // Create an IHXSockAddr
        IHXSockAddr* pAddr = NULL;
        retVal = CreateSockAddr(&pAddr);
        if (SUCCEEDED(retVal))
        {
            // Convert address and port from native to net order
            UINT32 ulAddr = m_ulUDPReadAddress;
            UINT16 usPort = m_usUDPReadPort;
            if (!m_bBigEndian)
            {
                ulAddr = CHXShimUtils::SwapUINT32(ulAddr);
                usPort = CHXShimUtils::SwapUINT16(usPort);
            }
            // Set these into the address
            HXSockUtil::SetAddrNetOrder(pAddr, AF_INET, &ulAddr, usPort);
            // Now read the buffer
            IHXBuffer* pBuffer = NULL;
            retVal = DoRead(&pBuffer, bPeek);
            if (SUCCEEDED(retVal))
            {
                // AddRef the out parameters
                pBuffer->AddRef();
                pAddr->AddRef();
                // Assign the out parameters
                *ppBuffer = pBuffer;
                *ppAddr   = pAddr;
            }
            HX_RELEASE(pBuffer);
        }
        HX_RELEASE(pAddr);
    }
    else
    {
        HX_ASSERT(FALSE);
        retVal = HXR_INVALID_PARAMETER;
    }

    return retVal;
}

void CHXClientSocketShim::SocketCallbackProc(void* pArg)
{
    if (pArg)
    {
        CHXClientSocketShim* pSocket = (CHXClientSocketShim*) pArg;
        pSocket->HandleCallback();
    }
}

