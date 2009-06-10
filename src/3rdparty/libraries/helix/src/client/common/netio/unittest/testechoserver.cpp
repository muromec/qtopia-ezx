/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved. 
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
#include "minicntx.h"
#include "hxnet.h"
#include "hxslist.h"
#include "hxthread.h"
#include "baseobj.h"
#include "pckunpck.h"
#include "nestbuff.h"
#include "hxtick.h"
#if defined(_WINDOWS)
#include <conio.h>
#endif
#include "hxsockutil.h"
#include "testbase.h"
#include "testechoserver.h"

#define TEST_ECHOSERVER_READSIZE 2048

TestEchoServer::TestEchoServer()
{
    m_lRefCount            = 0;
    m_pContext             = NULL;
    m_pNetServices         = NULL;
    m_pListeningSocket     = NULL;
    m_pConnectionList      = NULL;
    m_pConnectionListMutex = NULL;
}

TestEchoServer::~TestEchoServer()
{
    ClearConnectionList();
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pNetServices);
    HX_RELEASE(m_pListeningSocket);
    HX_DELETE(m_pConnectionList);
    HX_DELETE(m_pConnectionListMutex);
}

HX_RESULT TestEchoServer::RunTest(MiniContext*    pContext,
                                  IHXNetServices* pNetServices,
                                  int             argc,
                                  char**          argv)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pContext && pNetServices &&
        (argc == 3 || argc == 4))
    {
        // Save the IHXNetServices interface
        HX_RELEASE(m_pNetServices);
        m_pNetServices = pNetServices;
        m_pNetServices->AddRef();
        // Get the port string
        const char* pszPort = argv[2];
        // Are we supposed to use IPv6?
        HXBOOL bUseIPv6 = FALSE;
        if (argc == 4)
        {
            INT32 lUseIPv6 = (INT32) atol(argv[3]);
            bUseIPv6       = (lUseIPv6 ? TRUE : FALSE);
        }
        // Create the connection list mutex
#if defined(THREADS_SUPPORTED)
        HXMutex::MakeMutex(m_pConnectionListMutex);
#else
        HXMutex::MakeStubMutex(m_pConnectionListMutex);
#endif
        // Get the IUnknown interface from the MiniContext
        HX_RELEASE(m_pContext);
        retVal = pContext->QueryInterface(IID_IUnknown, (void**) &m_pContext);
        if (SUCCEEDED(retVal))
        {
            // Create a listening socket
            HX_RELEASE(m_pListeningSocket);
            retVal = m_pNetServices->CreateListeningSocket(&m_pListeningSocket);
            if (SUCCEEDED(retVal))
            {
                // Get our IHXListeningSocketResponse interface
                IHXListeningSocketResponse* pResponse = NULL;
                retVal = QueryInterface(IID_IHXListeningSocketResponse, (void**) &pResponse);
                if (SUCCEEDED(retVal))
                {
                    // Which family should we use?
                    HXSockFamily eSockFamily = (bUseIPv6 ? HX_SOCK_FAMILY_IN6 : HX_SOCK_FAMILY_IN4);
                    // Init the listening socket
                    retVal = m_pListeningSocket->Init(eSockFamily,
                                                      HX_SOCK_TYPE_TCP,
                                                      HX_SOCK_PROTO_ANY,
                                                      pResponse);
                    if (SUCCEEDED(retVal))
                    {
                        // Create an address on any interface for a specific port
                        IHXSockAddr* pAddr = NULL;
                        retVal = HXSockUtil::CreateAnyAddr(m_pNetServices, eSockFamily, pszPort, pAddr);
                        if (SUCCEEDED(retVal))
                        {
                            // Output some status info
                            fprintf(stdout, "%lu\tCalling IHXListeningSocket::Listen() on any interface, port %s\n",
                                    HX_GET_BETTERTICKCOUNT(), pszPort);
                            // Now listen on this address. Arbitrarily
                            // choose a backlog count of 4.
                            retVal = m_pListeningSocket->Listen(pAddr, 4);
                            if (SUCCEEDED(retVal))
                            {
                                // Loop on processing the mini-context until
                                // the user presses a 'q' key
                                HXBOOL   bQuit       = FALSE;
                                UINT32 ulStartTick = HX_GET_BETTERTICKCOUNT();
                                while (!bQuit)
                                {
                                    // Process the mini-context
                                    pContext->Process(MiniContext::MCNTX_BLOCK_NEVER);
                                    // Check the console (Windows only for now)
                                    // On other platforms we just timeout after 5 minutes
#if defined(_WINDOWS)
                                    if (_kbhit())
                                    {
                                        char c = _getch();
                                        if (c == 'q')
                                        {
                                            break;
                                        }
                                    }
#else
                                    UINT32 ulTick = HX_GET_BETTERTICKCOUNT();
                                    if (ulTick - ulStartTick > 300000)
                                    {
                                        break;
                                    }
#endif
                                }
                                // Shutdown the server
                                Shutdown();
                            }
                        }
                        HX_RELEASE(pAddr);
                    }
                }
                HX_RELEASE(pResponse);
            }
        }
    }

    return retVal;
}

STDMETHODIMP TestEchoServer::QueryInterface(REFIID riid, void** ppvObj)
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
            *ppvObj = (IUnknown*) (IHXListeningSocketResponse*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXListeningSocketResponse))
        {
            AddRef();
            *ppvObj = (IHXListeningSocketResponse*) this;
            retVal  = HXR_OK;
        }
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) TestEchoServer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) TestEchoServer::Release()
{
    INT32 rc = InterlockedDecrement(&m_lRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return (ULONG32) rc;
}

STDMETHODIMP TestEchoServer::OnConnection()
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pListeningSocket)
    {
        // Get the socket and address on which the
        // connection was made
        IHXSocket*   pSocket   = NULL;
        IHXSockAddr* pSockAddr = NULL;
        retVal = m_pListeningSocket->Accept(&pSocket, &pSockAddr);
        if (SUCCEEDED(retVal))
        {
            // Get the socket address and port for display
            char szAddr[HX_ADDRSTRLEN];
            memset(szAddr, 0, HX_ADDRSTRLEN);
            pSockAddr->GetAddr(szAddr);
            char szPort[HX_PORTSTRLEN];
            memset(szPort, 0, HX_PORTSTRLEN);
            pSockAddr->GetPort(szPort);
            // Output some status info
            fprintf(stdout, "%lu\tOnConnection() New connection on (%s,%s)\n",
                    HX_GET_BETTERTICKCOUNT(), szAddr, szPort);
            // Create a new TestEchoServerConnection object
            TestEchoServerConnection* pConn = new TestEchoServerConnection();
            if (pConn)
            {
                // AddRef the object
                pConn->AddRef();
                // Init the object
                retVal = pConn->Init(m_pContext, this, pSocket, pSockAddr);
                if (SUCCEEDED(retVal))
                {
                    // Lock the mutex
                    if (m_pConnectionListMutex) m_pConnectionListMutex->Lock();
                    // Have we created the list yet?
                    if (!m_pConnectionList)
                    {
                        m_pConnectionList = new CHXSimpleList();
                    }
                    if (m_pConnectionList)
                    {
                        // AddRef the connection before going on the list
                        pConn->AddRef();
                        // Add it to the list
                        m_pConnectionList->AddTail((void*) pConn);
                    }
                    else
                    {
                        retVal = HXR_OUTOFMEMORY;
                    }
                    // Unlock the mutex
                    if (m_pConnectionListMutex) m_pConnectionListMutex->Unlock();
                }
            }
            else
            {
                retVal = HXR_OUTOFMEMORY;
            }
            HX_RELEASE(pConn);
        }
        HX_RELEASE(pSocket);
        HX_RELEASE(pSockAddr);
    }

    return retVal;
}

STDMETHODIMP TestEchoServer::OnClosed()
{
    HX_RESULT retVal = HXR_OK;

    return retVal;
}

HX_RESULT TestEchoServer::NotifyConnectionClosed(TestEchoServerConnection* pConnect)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pConnect && m_pConnectionList)
    {
        // Output some status info
        fprintf(stdout, "%lu\tNotifyConnectionClosed(0x%08x)\n",
                HX_GET_BETTERTICKCOUNT(), pConnect);
        // Lock the mutex
        if (m_pConnectionListMutex) m_pConnectionListMutex->Lock();
        // Find the connection
        LISTPOSITION pos = m_pConnectionList->Find((void*) pConnect);
        if (pos)
        {
            // Remove the connection
            m_pConnectionList->RemoveAt(pos);
        }
        // Unlock the mutex
        if (m_pConnectionListMutex) m_pConnectionListMutex->Unlock();
        // Release the list's ref
        HX_RELEASE(pConnect);
    }

    return retVal;
}

void TestEchoServer::Shutdown()
{
    // Output some status info
    fprintf(stdout, "%lu\tShutdown()\n", HX_GET_BETTERTICKCOUNT());
    ClearConnectionList();
    if (m_pListeningSocket)
    {
        m_pListeningSocket->Close();
    }
}

void TestEchoServer::ClearConnectionList()
{
    if (m_pConnectionList)
    {
        // Lock the mutex
        if (m_pConnectionListMutex) m_pConnectionListMutex->Lock();
        // Remove all entries
        while (m_pConnectionList->GetCount() > 0)
        {
            TestEchoServerConnection* pConn =
                (TestEchoServerConnection*) m_pConnectionList->RemoveHead();
            if (pConn)
            {
                pConn->Abort();
            }
            HX_RELEASE(pConn);
        }
        // Unlock the mutex
        if (m_pConnectionListMutex) m_pConnectionListMutex->Unlock();
    }
}

TestEchoServerConnection::TestEchoServerConnection()
{
    m_lRefCount   = 0;
    m_pContext    = NULL;
    m_pEchoServer = NULL;
    m_pSocket     = NULL;
    m_pSockAddr   = NULL;
    m_pReadBuffer = NULL;
    m_ulBytesRead = 0;
    m_bOKToWrite  = FALSE;
}

TestEchoServerConnection::~TestEchoServerConnection()
{
    if (m_pSocket) m_pSocket->Close();
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pEchoServer);
    HX_RELEASE(m_pSocket);
    HX_RELEASE(m_pSockAddr);
    HX_RELEASE(m_pReadBuffer);
}


STDMETHODIMP TestEchoServerConnection::QueryInterface(REFIID riid, void** ppvObj)
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
            *ppvObj = (IUnknown*) (IHXSocketResponse*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXSocketResponse))
        {
            AddRef();
            *ppvObj = (IHXSocketResponse*) this;
            retVal  = HXR_OK;
        }
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) TestEchoServerConnection::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) TestEchoServerConnection::Release()
{
    INT32 rc = InterlockedDecrement(&m_lRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return (ULONG32) rc;
}

STDMETHODIMP TestEchoServerConnection::EventPending(UINT32 uEvent)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    // Output some status info
    CHXString cStr;
    TestBase::MakeSockEventString(uEvent, cStr);
    fprintf(stdout, "%lu\tEventPending(%s)\n",
            HX_GET_BETTERTICKCOUNT(), (const char*) cStr);

    switch (uEvent)
    {
        case HX_SOCK_EVENT_READ:
            {
                // Clear the return value
                retVal = HXR_OK;
                // Read one time only
                DoRead(1, "EventPending(HX_SOCK_EVENT_READ): ");
                // Write out everything we've received
                WriteOutEverything();
            }
            break;
        case HX_SOCK_EVENT_WRITE:
            {
                // Clear the return value
                retVal = HXR_OK;
                // Set the flag saying it's OK to write
                m_bOKToWrite = TRUE;
                // Write out everything we've received so far
                WriteOutEverything();
            }
            break;
        case HX_SOCK_EVENT_CLOSE:
            {
                // If we get a close event, then we should try to
                // read any remaining data from the socket
                // Do at most 10 reads
                DoRead(10, "EventPending(HX_SOCK_EVENT_CLOSE): ");
                // Notify the TestEchoServer that we closed
                if (m_pEchoServer)
                {
                    m_pEchoServer->NotifyConnectionClosed(this);
                }
            }
            break;
        case HX_SOCK_EVENT_CONNECT:
            {
                // Clear the return value
                retVal = HXR_OK;
            }
            break;
    }

    return retVal;
}

HX_RESULT TestEchoServerConnection::Init(IUnknown*       pContext,
                                         TestEchoServer* pServer,
                                         IHXSocket*      pSocket,
                                         IHXSockAddr*    pAddr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pServer && pSocket && pAddr)
    {
        // Save the member vars
        HX_RELEASE(m_pContext);
        m_pContext = pContext;
        m_pContext->AddRef();
        HX_RELEASE(m_pEchoServer);
        m_pEchoServer = pServer;
        m_pEchoServer->AddRef();
        HX_RELEASE(m_pSocket);
        m_pSocket = pSocket;
        m_pSocket->AddRef();
        HX_RELEASE(m_pSockAddr);
        m_pSockAddr = pAddr;
        m_pSockAddr->AddRef();
        // Create a read buffer
        HX_RELEASE(m_pReadBuffer);
        retVal = CreateBufferCCF(m_pReadBuffer, m_pContext);
        if (SUCCEEDED(retVal))
        {
            // Size the buffer
            retVal = m_pReadBuffer->SetSize(TEST_ECHOSERVER_READSIZE);
            if (SUCCEEDED(retVal))
            {
                // Get our IHXSocketResponse interface
                IHXSocketResponse* pResponse = NULL;
                retVal = QueryInterface(IID_IHXSocketResponse, (void**) &pResponse);
                if (SUCCEEDED(retVal))
                {
                    // Set the response on the socket
                    retVal = m_pSocket->SetResponse(pResponse);
                    if (SUCCEEDED(retVal))
                    {
                        // Select what events we want to see
                        retVal = m_pSocket->SelectEvents(HX_SOCK_EVENT_WRITE |
                                                         HX_SOCK_EVENT_READ  |
                                                         HX_SOCK_EVENT_CLOSE |
                                                         HX_SOCK_EVENT_CONNECT);
                    }
                }
                HX_RELEASE(pResponse);
            }
        }
    }

    return retVal;
}

void TestEchoServerConnection::Abort()
{
    // Close the socket
    if (m_pSocket)
    {
        m_pSocket->Close();
    }
}

void TestEchoServerConnection::DoRead(UINT32 ulNumReads, const char* pszDbgStr)
{
    if (m_pSocket)
    {
        for (UINT32 i = 0; i < ulNumReads; i++)
        {
            if (m_ulBytesRead < m_pReadBuffer->GetSize())
            {
                // Create a nested buffer, taking up
                // all the remaining space in the read buffer
                IHXBuffer* pNest = NULL;
                HX_RESULT rv = CHXNestedBuffer::CreateNestedBuffer(m_pReadBuffer,
                                                                   m_ulBytesRead,
                                                                   m_pReadBuffer->GetSize() - m_ulBytesRead,
                                                                   pNest);
                if (SUCCEEDED(rv))
                {
                    // Read from the socket
                    INT32 lBytesRead = m_pSocket->Read(pNest);
                    if (lBytesRead > 0)
                    {
                        // We successfully read, so update the 
                        // number of bytes read
                        m_ulBytesRead += (UINT32) lBytesRead;
                    }
                    else if (lBytesRead == 0)
                    {
                        // Zero bytes read, which means a graceful close
                        // on the socket
                        m_pSocket->Close();
                        // No more reads
                        break;
                    }
                    else
                    {
                        // Read() returned a socket error, so we should
                        // check the last error. If the last error is
                        // HX_SOCKERR_WOULDBLOCK, then we've read everything
                        // and shouldn't close the socket. If the last error
                        // is not HX_SOCKERR_WOULDBLOCK, then we should 
                        // close the socket. Either way, no more reads
                        INT32 lErr = m_pSocket->GetLastError();
                        if (lErr != HX_SOCKERR_WOULDBLOCK)
                        {
                            m_pSocket->Close();
                        }
                        break;
                    }
                }
                HX_RELEASE(pNest);
            }
        }
    }
}

void TestEchoServerConnection::WriteOutEverything()
{
    if (m_pSocket && m_ulBytesRead && m_bOKToWrite)
    {
        // Create a nested buffer which has the correct size
        IHXBuffer* pNest = NULL;
        HX_RESULT rv = CHXNestedBuffer::CreateNestedBuffer(m_pReadBuffer, 0, m_ulBytesRead, pNest);
        if (SUCCEEDED(rv))
        {
            // Write out the number of bytes to the socket
            INT32 lBytesWritten = m_pSocket->Write(pNest);
            if (lBytesWritten > 0)
            {
                // Reduce the bytes read by the bytes written
                m_ulBytesRead -= (UINT32) lBytesWritten;
            }
        }
        HX_RELEASE(pNest);
        // Clear the ok-to-write flag
        m_bOKToWrite = FALSE;
    }
}

