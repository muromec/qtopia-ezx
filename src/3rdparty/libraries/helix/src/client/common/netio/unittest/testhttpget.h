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

#ifndef TESTHTTPGET_H
#define TESTHTTPGET_H

// Forward declarations
typedef _INTERFACE IHXNetServices     IHXNetServices;
typedef _INTERFACE IHXSockAddr        IHXSockAddr;
typedef _INTERFACE IHXSocket          IHXSocket;
typedef _INTERFACE IHXResolve         IHXResolve;
typedef _INTERFACE IHXBuffer          IHXBuffer;
class MiniContext;
class CHXString;

class TestHTTPGet : public TestBase,
                    public IHXSocketResponse,
                    public IHXResolveResponse
{
public:
    TestHTTPGet();
    virtual ~TestHTTPGet();

    // TestBase methods
    virtual HX_RESULT RunTest(MiniContext*    pContext,
                              IHXNetServices* pNetServices,
                              int             argc,
                              char**          argv);

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXSocketResponse
    STDMETHOD(EventPending) (THIS_ UINT32 uEvent);

    // IHXResolveResponse
    STDMETHOD(GetAddrInfoDone) (THIS_ HX_RESULT status, IHXSockAddr** ppAddrVec, UINT32 nVecLen);
    STDMETHOD(GetNameInfoDone) (THIS_ HX_RESULT status, const char* pszNode, const char* pszService);
protected:
    enum TestState
    {
        TestStateBegin,
        TestStateGetAddrInfoDonePending,
        TestStateConnectEventPending,
        TestStateConnected,
        TestStateDone
    };

    INT32           m_lRefCount;
    IHXNetServices* m_pNetServices;
    TestState       m_eState;
    IHXSocket*      m_pSocket;
    IHXResolve*     m_pResolve;
    IHXBuffer*      m_pHostStr;
    IHXBuffer*      m_pPortStr;
    IHXBuffer*      m_pWriteBuffer;
    IHXBuffer*      m_pReadBuffer;
    IHXBuffer*      m_pOutputFileStr;
    UINT32          m_ulBytesWritten;

    HX_RESULT CreateAndInitSocket(IHXNetServices* pNetServices, IHXSocket** ppSocket, HXBOOL bUseIPv6);
    HX_RESULT CreateAndInitResolve(IHXNetServices* pNetServices, IHXSocket* pSocket, IHXResolve** ppResolve);
    HX_RESULT ParseHostPortGet(const char* pszURL, IUnknown* pContext, REF(IHXBuffer*) rpHostStr,
                               REF(IHXBuffer*) rpPortStr, REF(IHXBuffer*) rpGetStr);
    void      DoRead(UINT32 ulNumReads, const char* pszDbgStr);

    static const char* const m_pszWriteStr;
};

#endif /* #ifndef TESTHTTPGET_H */
