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
#include "hxurl.h"
#include "pckunpck.h"
#include "hxtick.h"
#include "hxsockutil.h"
#include "testbase.h"
#include "testhttpget.h"

#define TEST_HTTPGET_READSIZE    2048

// To construct the write buffer, we simply need to
// prepend "GET " and the fullpath of the URL to
// this string
const char* const TestHTTPGet::m_pszWriteStr = " HTTP/1.0\r\n"
                                               "Accept: */*\r\n"
                                               "Accept-Language: en\r\n"
                                               "Connection: Keep-Alive\r\n"
                                               "\r\n";

TestHTTPGet::TestHTTPGet()
{
    m_lRefCount      = 0;
    m_pNetServices   = NULL;
    m_eState         = TestStateBegin;
    m_pSocket        = NULL;
    m_pResolve       = NULL;
    m_pHostStr       = NULL;
    m_pPortStr       = NULL;
    m_pWriteBuffer   = NULL;
    m_pReadBuffer    = NULL;
    m_pOutputFileStr = NULL;
    m_ulBytesWritten = 0;
}

TestHTTPGet::~TestHTTPGet()
{
    if (m_pSocket) m_pSocket->Close();
    HX_RELEASE(m_pNetServices);
    HX_RELEASE(m_pSocket);
    HX_RELEASE(m_pResolve);
    HX_RELEASE(m_pHostStr);
    HX_RELEASE(m_pPortStr);
    HX_RELEASE(m_pWriteBuffer);
    HX_RELEASE(m_pReadBuffer);
    HX_RELEASE(m_pOutputFileStr);
}

HX_RESULT TestHTTPGet::RunTest(MiniContext*    pContext,
                               IHXNetServices* pNetServices,
                               int             argc,
                               char**          argv)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pContext && pNetServices &&
        (argc == 3 || argc == 4 || argc == 5))
    {
        // Save the net services interface
        HX_RELEASE(m_pNetServices);
        m_pNetServices = pNetServices;
        m_pNetServices->AddRef();
        // Get the URL
        char* pszURL = argv[2];
        // Get the output file
        const char* pszOutputFile = "httpget_output.txt";
        if (argc == 4)
        {
            pszOutputFile = (const char*) argv[3];
        }
        // Are we supposed to use IPv6?
        HXBOOL bUseIPv6 = FALSE;
        if (argc == 5)
        {
            INT32 lUseIPv6 = (INT32) atol(argv[4]);
            bUseIPv6       = (lUseIPv6 ? TRUE : FALSE);
        }
        // Get the mini-context's IUknown interface
        IUnknown* pUnk = NULL;
        retVal = pContext->QueryInterface(IID_IUnknown, (void**) &pUnk);
        if (SUCCEEDED(retVal))
        {
            // Parse the URL and construct
            // the HTTP GET string
            HX_RELEASE(m_pHostStr);
            HX_RELEASE(m_pPortStr);
            HX_RELEASE(m_pWriteBuffer);
            retVal = ParseHostPortGet(pszURL, pUnk, m_pHostStr, m_pPortStr, m_pWriteBuffer);
            if (SUCCEEDED(retVal))
            {
                // Create our read buffer
                HX_RELEASE(m_pReadBuffer);
                retVal = CreateBufferCCF(m_pReadBuffer, pUnk);
                if (SUCCEEDED(retVal))
                {
                    // Size our read buffer
                    retVal = m_pReadBuffer->SetSize(TEST_HTTPGET_READSIZE);
                    if (SUCCEEDED(retVal))
                    {
                        // Create our output file string buffer
                        HX_RELEASE(m_pOutputFileStr);
                        retVal = CreateStringBufferCCF(m_pOutputFileStr, pszOutputFile, pUnk);
                        if (SUCCEEDED(retVal))
                        {
                            // Set the initial state
                            m_eState = TestStateBegin;
                            // Set the initial members
                            m_ulBytesWritten = 0;
                            // Clear the return value
                            retVal = HXR_OK;
                            // Loop until we reach the TestStateDone state
                            while (m_eState != TestStateDone && SUCCEEDED(retVal))
                            {
                                // Switch based on state
                                switch (m_eState)
                                {
                                    case TestStateBegin:
                                        {
                                            // Create and initialize the IHXSocket
                                            if (m_pSocket) m_pSocket->Close();
                                            HX_RELEASE(m_pSocket);
                                            retVal = CreateAndInitSocket(pNetServices, &m_pSocket, bUseIPv6);
                                            if (SUCCEEDED(retVal))
                                            {
                                                // Create and initialize the IHXResolve
                                                HX_RELEASE(m_pResolve);
                                                retVal = CreateAndInitResolve(pNetServices, m_pSocket, &m_pResolve);
                                                if (SUCCEEDED(retVal))
                                                {
                                                    // Set the state
                                                    m_eState = TestStateGetAddrInfoDonePending;
                                                    // Output some status info
                                                    fprintf(stdout, "%lu\tRunTest(): Calling IHXResolve::GetAddrInfo(%s,%s,NULL)\n",
                                                            HX_GET_BETTERTICKCOUNT(),
                                                            (const char*) m_pHostStr->GetBuffer(),
                                                            (const char*) m_pPortStr->GetBuffer());
                                                    // Call GetAddrInfo to resolve the URL
                                                    retVal = m_pResolve->GetAddrInfo((const char*) m_pHostStr->GetBuffer(),
                                                                                     (const char*) m_pPortStr->GetBuffer(),
                                                                                     NULL);
                                                }
                                            }
                                        }
                                        break;
                                    case TestStateGetAddrInfoDonePending:
                                    case TestStateConnectEventPending:
                                    case TestStateConnected:
                                        {
                                            // Pump the context
                                            retVal = pContext->Process(MiniContext::MCNTX_BLOCK_NEVER);
                                        }
                                        break;
                                }
                            }
                            // Clean up
                            HX_RELEASE(m_pResolve);
                            if (m_pSocket)
                            {
                                m_pSocket->Close();
                            }
                            HX_RELEASE(m_pSocket);
                        }
                    }
                }
            }
        }
        HX_RELEASE(pUnk);
    }

    return retVal;
}

STDMETHODIMP TestHTTPGet::QueryInterface(REFIID riid, void** ppvObj)
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
        else if (IsEqualIID(riid, IID_IHXResolveResponse))
        {
            AddRef();
            *ppvObj = (IHXResolveResponse*) this;
            retVal  = HXR_OK;
        }
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) TestHTTPGet::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) TestHTTPGet::Release()
{
    INT32 rc = InterlockedDecrement(&m_lRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return (ULONG32) rc;
}

STDMETHODIMP TestHTTPGet::EventPending(UINT32 uEvent)
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
            }
            break;
        case HX_SOCK_EVENT_WRITE:
            {
                // Clear the return value
                retVal = HXR_OK;
                // Do we have anything else left to write?
                if (m_ulBytesWritten < m_pWriteBuffer->GetSize())
                {
                    // Output some status info
                    fprintf(stdout, "%lu\tEventPending(HX_SOCK_EVENT_WRITE): Calling IHXSocket::Write(0x%08x) with a buffer of size %lu\n",
                            HX_GET_BETTERTICKCOUNT(), m_pWriteBuffer, m_pWriteBuffer->GetSize());
                    // We can write out our buffer
                    INT32 lBytesWritten = m_pSocket->Write(m_pWriteBuffer);
                    // Output some status info
                    fprintf(stdout, "%lu\tEventPending(HX_SOCK_EVENT_WRITE): IHXSocket::Write(0x%08x) returned %ld\n",
                            HX_GET_BETTERTICKCOUNT(), m_pWriteBuffer, lBytesWritten);
                    // Force bytes written to be the entire buffer
                    m_ulBytesWritten = m_pWriteBuffer->GetSize();
                }
            }
            break;
        case HX_SOCK_EVENT_CONNECT:
            {
                if (m_eState == TestStateConnectEventPending)
                {
                    // Clear the return value
                    retVal = HXR_OK;
                    // Check the last error on the socket
                    INT32 lErr = (INT32) m_pSocket->GetLastError();
                    if (lErr == HX_SOCKERR_NONE)
                    {
                        // Output some status info
                        fprintf(stdout, "%lu\tEventPending(HX_SOCK_EVENT_CONNECT): Successful connect, changing state to TestStateConnected\n",
                                HX_GET_BETTERTICKCOUNT());
                        // Change the state
                        m_eState = TestStateConnected;
                    }
                    else
                    {
                        // Output some status info
                        CHXString cErr;
                        TestBase::MakeSockErrString(lErr, cErr);
                        fprintf(stdout, "%lu\tEventPending(HX_SOCK_EVENT_CONNECT): IHXSocket::GetLastError() = %s, so closing socket and changing state to TestStateDone\n",
                                HX_GET_BETTERTICKCOUNT(), (const char*) cErr);
                        // We failed to connect, so close the socket
                        m_pSocket->Close();
                        // Change the state
                        m_eState = TestStateDone;
                    }
                }
            }
            break;
        case HX_SOCK_EVENT_CLOSE:
            {
                // If we get a close event, then we should try to
                // read any remaining data from the socket
                // Do at most 10 reads
                DoRead(10, "EventPending(HX_SOCK_EVENT_CLOSE): ");
            }
            break;
    }

    return retVal;
}

STDMETHODIMP TestHTTPGet::GetAddrInfoDone(HX_RESULT status, IHXSockAddr** ppAddrVec, UINT32 nVecLen)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    // Output some status info
    fprintf(stdout, "%lu\tGetAddrInfoDone(0x%08x,0x%08x,%lu)\n",
            HX_GET_BETTERTICKCOUNT(), status, ppAddrVec, nVecLen);

    if (m_eState == TestStateGetAddrInfoDonePending)
    {
        if (SUCCEEDED(status) && nVecLen > 0)
        {
            // Convert these addresses to ones 
            // compatible with our socket
            IHXSockAddr** ppConvSockAddr = NULL;
            retVal = HXSockUtil::AllocAddrVec(ppAddrVec,
                                              nVecLen,
                                              ppConvSockAddr,
                                              m_pSocket->GetFamily(),
                                              TRUE,
                                              m_pNetServices);
            if (SUCCEEDED(retVal))
            {
                // Set the state
                m_eState = TestStateConnectEventPending;
                // Output some status info
                fprintf(stdout, "%lu\tGetAddrInfoDone(): Calling IHXSocket::ConnectToAny(%lu,0x%08x)\n",
                        HX_GET_BETTERTICKCOUNT(), nVecLen, ppConvSockAddr);
                // Now call ConnectToAny
                retVal = m_pSocket->ConnectToAny(nVecLen, ppConvSockAddr);
            }
            // Free the addresses we converted
            HXSockUtil::FreeAddrVec(ppConvSockAddr, nVecLen);
        }
        else
        {
            // We failed to resolve, so set the state to done
            m_eState = TestStateDone;
            // Clear the return value
            retVal = HXR_OK;
        }
    }

    return retVal;
}

STDMETHODIMP TestHTTPGet::GetNameInfoDone(HX_RESULT status, const char* pszNode, const char* pszService)
{
    return HXR_NOTIMPL;
}

HX_RESULT TestHTTPGet::CreateAndInitSocket(IHXNetServices* pNetServices, IHXSocket** ppSocket, HXBOOL bUseIPv6)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pNetServices && ppSocket)
    {
        // Create the socket
        IHXSocket* pSocket = NULL;
        retVal = pNetServices->CreateSocket(&pSocket);
        if (SUCCEEDED(retVal))
        {
            // Which family should we use?
            HXSockFamily eSockFamily = (bUseIPv6 ? HX_SOCK_FAMILY_IN6 : HX_SOCK_FAMILY_IN4);
            // Init the socket
            retVal = pSocket->Init(eSockFamily, HX_SOCK_TYPE_TCP, HX_SOCK_PROTO_ANY);
            if (SUCCEEDED(retVal))
            {
                // Get our IHXSocketResponse interface
                IHXSocketResponse* pResponse = NULL;
                retVal = QueryInterface(IID_IHXSocketResponse, (void**) &pResponse);
                if (SUCCEEDED(retVal))
                {
                    // Set the socket response interface
                    retVal = pSocket->SetResponse(pResponse);
                    if (SUCCEEDED(retVal))
                    {
                        // Select which events we want
                        retVal = pSocket->SelectEvents(HX_SOCK_EVENT_CONNECT |
                                                       HX_SOCK_EVENT_WRITE   |
                                                       HX_SOCK_EVENT_READ    |
                                                       HX_SOCK_EVENT_CLOSE);
                        if (SUCCEEDED(retVal))
                        {
                            // AddRef the socket before going out
                            pSocket->AddRef();
                            // Assign the out parameter
                            *ppSocket = pSocket;
                        }
                    }
                }
                HX_RELEASE(pResponse);
            }
        }
        HX_RELEASE(pSocket);
    }

    return retVal;
}

HX_RESULT TestHTTPGet::CreateAndInitResolve(IHXNetServices* pNetServices, IHXSocket* pSocket, IHXResolve** ppResolve)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pNetServices && pSocket && ppResolve)
    {
        // Create the IHXResolve
        IHXResolve* pResolve = NULL;
        retVal = pNetServices->CreateResolver(&pResolve);
        if (SUCCEEDED(retVal))
        {
            // Get our own IHXResolveResponse interface
            IHXResolveResponse* pResponse = NULL;
            retVal = QueryInterface(IID_IHXResolveResponse, (void**) &pResponse);
            if (SUCCEEDED(retVal))
            {
                // Init the resolver
                retVal = pResolve->Init(pResponse);
                if (SUCCEEDED(retVal))
                {
                    // AddRef the resolver before going out
                    pResolve->AddRef();
                    // Assign the out parameter
                    *ppResolve = pResolve;
                }
            }
            HX_RELEASE(pResponse);
        }
        HX_RELEASE(pResolve);
    }

    return retVal;
}

HX_RESULT TestHTTPGet::ParseHostPortGet(const char* pszURL, IUnknown* pContext, REF(IHXBuffer*) rpHostStr,
                                        REF(IHXBuffer*) rpPortStr, REF(IHXBuffer*) rpGetStr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pszURL)
    {
        // Parse the URL
        CHXURL cURL(pszURL);
        // Make sure the parse succeeded and that
        // the URL begins with http://
        if (SUCCEEDED(cURL.GetLastError()) &&
            cURL.GetProtocol() == httpProtocol)
        {
            // Get the properties
            IHXValues* pProps = cURL.GetProperties();
            if (pProps)
            {
                // Get the host string
                HX_RELEASE(rpHostStr);
                retVal = pProps->GetPropertyBuffer(PROPERTY_HOST, rpHostStr);
                if (SUCCEEDED(retVal))
                {
                    // Get the port
                    UINT32 ulPort = 0;
                    retVal = pProps->GetPropertyULONG32(PROPERTY_PORT, ulPort);
                    if (SUCCEEDED(retVal))
                    {
                        // Convert it to a string
                        char szPort[16]; /* Flawfinder: ignore */
                        sprintf(szPort, "%lu", ulPort); /* Flawfinder: ignore */
                        // Put the string into a buffer
                        HX_RELEASE(rpPortStr);
                        retVal = CreateStringBufferCCF(rpPortStr, szPort, pContext);
                        if (SUCCEEDED(retVal))
                        {
                            // Get the fullpath string
                            IHXBuffer* pFullPathStr = NULL;
                            retVal = pProps->GetPropertyBuffer(PROPERTY_FULLPATH, pFullPathStr);
                            if (SUCCEEDED(retVal))
                            {
                                // Construct the HTTP GET string
                                CHXString cGet;
                                cGet  = "GET ";
                                cGet += (const char*) pFullPathStr->GetBuffer();
                                cGet += m_pszWriteStr;
                                // Make the get string into a buffer
                                HX_RELEASE(rpGetStr);
                                retVal = CreateStringBufferCCF(rpGetStr, (const char*) cGet, pContext);
                            }
                        }
                    }
                }
            }
            HX_RELEASE(pProps);
        }
    }

    return retVal;
}

void TestHTTPGet::DoRead(UINT32 ulNumReads, const char* pszDbgStr)
{
    for (UINT32 i = 0; i < ulNumReads; i++)
    {
        // Output some status info
        fprintf(stdout, "%lu\t%sCalling IHXSocket::Read with buffer of size %lu\n",
                HX_GET_BETTERTICKCOUNT(), pszDbgStr, m_pReadBuffer->GetSize());
        // Read into our read buffer
        INT32 lBytesRead = m_pSocket->Read(m_pReadBuffer);
        if (lBytesRead > 0)
        {
            // Output some status info
            fprintf(stdout, "%lu\t%sRead %ld bytes\n", HX_GET_BETTERTICKCOUNT(), pszDbgStr, lBytesRead);
            // Write out to our output file
            FILE* fp = fopen((const char*) m_pOutputFileStr->GetBuffer(), "ab");
            if (fp)
            {
                fwrite((const void*) m_pReadBuffer->GetBuffer(), 1, lBytesRead, fp);
            }
            fclose(fp);
        }
        else if (lBytesRead == 0)
        {
            // Output some status info
            fprintf(stdout, "%lu\t%sZERO-BYTE Read, Graceful close, closing socket\n",
                    HX_GET_BETTERTICKCOUNT(), pszDbgStr, lBytesRead);
            // Close the socket
            m_pSocket->Close();
            // Change the state
            m_eState = TestStateDone;
            // Don't attempt any more reads
            break;
        }
        else
        {
            // Probably means some error has occurred, so check
            // the last error
            UINT32 ulLastErr = m_pSocket->GetLastError();
            // Output some status info
            fprintf(stdout, "%lu\t%sRead() returned %ld, last error = %lu\n",
                    HX_GET_BETTERTICKCOUNT(), pszDbgStr, lBytesRead, ulLastErr);
            // If the last error is would block, then we'll
            // wait for another read. Otherwise, we close the socket
            if (ulLastErr != SOCKERR_WOULDBLOCK)
            {
                // Close the socket
                m_pSocket->Close();
                // Change the state
                m_eState = TestStateDone;
            }
            // Don't attempt any more reads
            break;
        }
    }
}

