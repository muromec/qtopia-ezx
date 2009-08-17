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

#ifndef TESTECHOSERVER_H
#define TESTECHOSERVER_H

// Forward declarations
typedef _INTERFACE IHXNetServices     IHXNetServices;
typedef _INTERFACE IHXSockAddr        IHXSockAddr;
typedef _INTERFACE IHXSocket          IHXSocket;
typedef _INTERFACE IHXBuffer          IHXBuffer;
typedef _INTERFACE IHXListeningSocket IHXListeningSocket;
class MiniContext;
class TestEchoServer;
class TestEchoServerConnection;
class CHXSimpleList;
class HXMutex;

class TestEchoServer : public TestBase,
                       public IHXListeningSocketResponse
{
public:
    TestEchoServer();
    virtual ~TestEchoServer();

    // TestBase methods
    virtual HX_RESULT RunTest(MiniContext*    pContext,
                              IHXNetServices* pNetServices,
                              int             argc,
                              char**          argv);

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXListeningSocketResponse methods
    STDMETHOD(OnConnection) (THIS);
    STDMETHOD(OnClosed)     (THIS);

    // TestEchoServer methods
    HX_RESULT NotifyConnectionClosed(TestEchoServerConnection* pConnect);
    void      Shutdown();
protected:
    INT32               m_lRefCount;
    IUnknown*           m_pContext;
    IHXNetServices*     m_pNetServices;
    IHXListeningSocket* m_pListeningSocket;
    CHXSimpleList*      m_pConnectionList;
    HXMutex*            m_pConnectionListMutex;

    void ClearConnectionList();
};

class TestEchoServerConnection : public IHXSocketResponse
{
public:
    TestEchoServerConnection();
    virtual ~TestEchoServerConnection();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXSocketResponse methods
    STDMETHOD(EventPending) (THIS_ UINT32 uEvent);

    // TestEchoServerConnection methods
    HX_RESULT Init(IUnknown* pContext, TestEchoServer* pServer,
                   IHXSocket* pSocket, IHXSockAddr* pAddr);
    void      Abort();
protected:
    INT32           m_lRefCount;
    IUnknown*       m_pContext;
    TestEchoServer* m_pEchoServer;
    IHXSocket*      m_pSocket;
    IHXSockAddr*    m_pSockAddr;
    IHXBuffer*      m_pReadBuffer;
    UINT32          m_ulBytesRead;
    HXBOOL            m_bOKToWrite;

    void DoRead(UINT32 ulNumReads, const char* pszDbgStr);
    void WriteOutEverything();
};

#endif /* #ifndef TESTECHOSERVER_H */
