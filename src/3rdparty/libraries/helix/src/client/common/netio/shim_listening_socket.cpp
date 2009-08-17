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
#include "hxtick.h"
#include "netbyte.h"
#include "hxsockutil.h"
#include "hxassert.h"
#include "shim_utils.h"
#include "shim_socket.h"
#include "shim_listening_socket.h"

CHXListeningSocketShim::CHXListeningSocketShim(IUnknown* pContext, 
                                               IHXNetServices* pNetServices,
                                               IHXNetworkServices* pOldNetServices)
{
    HX_ASSERT(pContext);
    HX_ASSERT(pNetServices);
    HX_ASSERT(pOldNetServices);

    m_lRefCount     = 0;
    m_bBigEndian    = TestBigEndian();
    m_pContext      = pContext;
    m_pNetServices  = pNetServices;
    m_pOldNetServices = pOldNetServices;
    m_eSockFamily   = HX_SOCK_FAMILY_NONE;
    m_eSockType     = HX_SOCK_TYPE_NONE;
    m_eSockProtocol = HX_SOCK_PROTO_NONE;
    m_pResponse     = NULL;
    m_pListenSocket = NULL;
    m_pTCPSocket    = NULL;
   
    m_pContext->AddRef();
    m_pNetServices->AddRef();
    m_pOldNetServices->AddRef();
}

CHXListeningSocketShim::~CHXListeningSocketShim()
{
    Close();
}

STDMETHODIMP CHXListeningSocketShim::QueryInterface(REFIID riid, void** ppvObj)
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
            *ppvObj = (IUnknown*) (IHXListeningSocket*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXListeningSocket))
        {
            AddRef();
            *ppvObj = (IHXListeningSocket*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXListenResponse))
        {
            AddRef();
            *ppvObj = (IHXListenResponse*) this;
            retVal  = HXR_OK;
        }
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CHXListeningSocketShim::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXListeningSocketShim::Release()
{
    INT32 rc = InterlockedDecrement(&m_lRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return (ULONG32) rc;
}

STDMETHODIMP CHXListeningSocketShim::Init(HXSockFamily f, HXSockType t, HXSockProtocol p,
                                          IHXListeningSocketResponse* pResponse)
{
    HX_RESULT retVal = HXR_FAIL;

    // Filter out families, types, and protocols
    // that the shim can't support
    if ((f == HX_SOCK_FAMILY_IN4 || f == HX_SOCK_FAMILY_INANY) &&
        (t == HX_SOCK_TYPE_UDP   || t == HX_SOCK_TYPE_TCP)     &&
         p == HX_SOCK_PROTO_ANY                                &&
         pResponse)
    {
        // Save the family, type, and protocol
        m_eSockFamily   = HX_SOCK_FAMILY_IN4;
        m_eSockType     = t;
        m_eSockProtocol = p;
        // Save the response interface
        HX_RELEASE(m_pResponse);
        m_pResponse = pResponse;
        m_pResponse->AddRef();
        // Clear the return value
        retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP CHXListeningSocketShim::Listen(IHXSockAddr* pAddr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pAddr)
    {
        // Create an IHXListenSocket
        HX_RELEASE(m_pListenSocket);
        retVal = m_pOldNetServices->CreateListenSocket(&m_pListenSocket);
        if (SUCCEEDED(retVal))
        {
            // Get the IN4 address (net order)
            UINT32 ulAddr = 0;
            retVal = HXSockUtil::GetIN4Address(pAddr, ulAddr);
            if (SUCCEEDED(retVal))
            {
                // Since IHXListenSocket::Init expects native
                // order, then if we are little-endian, then
                // we need to swap
                if (!m_bBigEndian)
                {
                    ulAddr = CHXShimUtils::SwapUINT32(ulAddr);
                }
                // Get the IN4 port (native order)
                UINT16 usPort = 0;
                retVal = HXSockUtil::GetIN4Port(pAddr, usPort);
                if (SUCCEEDED(retVal))
                {
                    // Get our IHXListenResponse interface
                    IHXListenResponse* pResponse = NULL;
                    retVal = QueryInterface(IID_IHXListenResponse, (void**) &pResponse);
                    if (SUCCEEDED(retVal))
                    {
                        // Initialize the IHXListenSocket. This causes
                        // it to start listening
                        retVal = m_pListenSocket->Init(ulAddr, usPort, pResponse);
                    }
                    HX_RELEASE(pResponse);
                }
            }
        }
    
    }

    return retVal;
}

STDMETHODIMP CHXListeningSocketShim::Accept(IHXSocket** ppNewSock, IHXSockAddr** ppSource)
{
    // This method is not needed anymore since 
    // the response interface's OnConnection() goes
    // ahead and does the function of Accept() before
    // it passes the socket back.
    return HXR_FAIL;
}

STDMETHODIMP CHXListeningSocketShim::Close()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pNetServices);
    HX_RELEASE(m_pOldNetServices);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pListenSocket);
    HX_RELEASE(m_pTCPSocket);

    return HXR_OK;
}

STDMETHODIMP CHXListeningSocketShim::GetOption(HXSockOpt name, UINT32* pval)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CHXListeningSocketShim::SetOption(HXSockOpt name, UINT32 val)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pListenSocket)
    {
        // Translate from 
        HX_SOCKET_OPTION eOpt = HX_SOCKOPT_REUSE_ADDR;
        retVal = CHXShimUtils::TranslateFromHXSockOpt(name, eOpt);
        if (SUCCEEDED(retVal))
        {
            // Get the IHXSetSocketOption interface
            IHXSetSocketOption* pSet = NULL;
            retVal = m_pListenSocket->QueryInterface(IID_IHXSetSocketOption, (void**) &pSet);
            if (SUCCEEDED(retVal))
            {
                // Set the option
                retVal = pSet->SetOption(eOpt, val);
            }
            HX_RELEASE(pSet);
        }
    }

    return retVal;
}

STDMETHODIMP CHXListeningSocketShim::NewConnection(HX_RESULT status, IHXTCPSocket* pTCPSocket)
{
    HX_RESULT retVal = HXR_FAIL;

    if (SUCCEEDED(status) && pTCPSocket && m_pResponse)
    {
        // Create the IHXSocket and IHXSockAddr
        IHXSocket*   pSocket = NULL;
        IHXSockAddr* pAddr   = NULL;
        retVal = CreateInitSocket(pTCPSocket, &pSocket, &pAddr);
        if (SUCCEEDED(retVal))
        {
            // Call the response interface
            retVal = m_pResponse->OnConnection(pSocket, pAddr);
        }
    }

    return retVal;
}

HX_RESULT CHXListeningSocketShim::CreateInitSocket(IHXTCPSocket* pTCPSocket, IHXSocket** ppSocket, IHXSockAddr** ppAddr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pTCPSocket && ppSocket && ppAddr && m_pContext && m_pNetServices)
    {
        // Get the TCP Socket's foreign address (native order)
        UINT32 ulAddr = 0;
        retVal = pTCPSocket->GetForeignAddress(ulAddr);
        if (SUCCEEDED(retVal))
        {
            // Get the TCP Socket's foreign port (native order)
            UINT16 usPort = 0;
            retVal = pTCPSocket->GetForeignPort(usPort);
            if (SUCCEEDED(retVal))
            {
                // Create an IHXSockAddr with these parameters
                IHXSockAddr* pAddr = NULL;
                retVal = HXSockUtil::CreateAddrIN4(m_pNetServices, ulAddr, usPort, pAddr);
                if (SUCCEEDED(retVal))
                {
                    // Create an CHXClientSocketShim object
                    CHXClientSocketShim* pSocket = new CHXClientSocketShim(m_pContext, m_pNetServices, m_pOldNetServices);
                    if (pSocket)
                    {
                        // AddRef the CHXClientSocketShim
                        pSocket->AddRef();
                        // Put the IHXTCPSocket into the CHXClientSocketShim
                        retVal = pSocket->Init(pTCPSocket);
                        if (SUCCEEDED(retVal))
                        {
                            // Since the respose interface was not set when the
                            // socket was connected, then it did not get a
                            // ConnectDone(), which would have kicked off
                            // a IHXTCPSocket::Read(). Therefore, we need to
                            // issue a callback which will kick off a read.
                            pSocket->ScheduleCallback(HX_SOCK_EVENT_CONNECT |
                                                      HX_SOCK_EVENT_WRITE   |
                                                      HX_SOCKET_SHIM_FLAG_ISSUEREAD);
                            // Get the IHXSocket interface
                            IHXSocket* pIHXSocket = NULL;
                            retVal = pSocket->QueryInterface(IID_IHXSocket, (void**) &pIHXSocket);
                            if (SUCCEEDED(retVal))
                            {
                                // AddRef the IHXSocket before it goes out
                                pIHXSocket->AddRef();
                                // Assign the IHXSocket out parameter
                                *ppSocket = pIHXSocket;
                                // AddRef the IHXSockAddr before it goes out
                                pAddr->AddRef();
                                // Assign the IHXSockAddr out parameter
                                *ppAddr = pAddr;
                            }
                            HX_RELEASE(pIHXSocket);
                        }
                    }
                    else
                    {
                        retVal = HXR_OUTOFMEMORY;
                    }
                    HX_RELEASE(pSocket);
                }
                HX_RELEASE(pAddr);
            }
        }
    }

    return retVal;
}
