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

#ifndef _OLDNETAPI_H
#define _OLDNETAPI_H

#include "nettypes.h"

class CHXOldListenSocket : public IHXListenSocket,
                           public IHXSocketResponse
{
private:   // Unimplemented
    CHXOldListenSocket(const CHXOldListenSocket&);
    CHXOldListenSocket& operator=(const CHXOldListenSocket&);

public:
    CHXOldListenSocket(IUnknown* punkContext);
    virtual ~CHXOldListenSocket(void);

    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXListenSocket
    STDMETHOD(Init)                 (THIS_ UINT32 uAddr, UINT16 uPort,
                                           IHXListenResponse* pResponse);

    // IHXSocketResponse
    STDMETHOD(EventPending)         (THIS_ UINT32 uEvent, HX_RESULT status);

private:
    void DoAccept(void);
    void DoClose(void);

protected:
    INT32                       m_nRefCount;
    IHXCommonClassFactory*      m_pCCF;
    IHXNetServices*             m_pNetSvc;
    IHXListenResponse*          m_pResponse;
    IHXSocket*                  m_pSock;
};

class CHXOldResolver : public IHXResolver,
                       public IHXResolveResponse
{
private:    // Unimplemented
    CHXOldResolver(const CHXOldResolver&);
    CHXOldResolver& operator=(const CHXOldResolver&);

public:
    CHXOldResolver(IUnknown* punkContext);
    virtual ~CHXOldResolver(void);

    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXResolver
    STDMETHOD(Init)                 (THIS_ IHXResolverResponse* pResponse);
    STDMETHOD(GetHostByName)        (THIS_ const char* pHost);

    // IHXResolveResponse
    STDMETHOD(GetAddrInfoDone)      (THIS_ HX_RESULT status,
                                           UINT32 nVecLen,
                                           IHXSockAddr** ppAddrVec);
    STDMETHOD(GetNameInfoDone)      (THIS_ HX_RESULT status,
                                           const char* pNode,
                                           const char* pService);

protected:
    INT32                       m_nRefCount;
    IHXCommonClassFactory*      m_pCCF;
    IHXNetServices*             m_pNetSvc;
    IHXResolverResponse*        m_pResponse;
    IHXResolve*                 m_pResolve;
};

class CHXOldTCPSocket : public IHXTCPSocket,
                        public IHXResolveResponse,
                        public IHXWouldBlock,
                        public IHXSetSocketOption,
                        public IHXSocketResponse
{
private:    // Unimplemented
    CHXOldTCPSocket(const CHXOldTCPSocket&);
    CHXOldTCPSocket& operator=(const CHXOldTCPSocket&);

public:
    CHXOldTCPSocket(IUnknown* punkContext, IHXSocket* pSock);
    virtual ~CHXOldTCPSocket(void);

    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXTCPSocket
    STDMETHOD(Init)                 (THIS_ IHXTCPResponse* pResponse);
    STDMETHOD(SetResponse)          (THIS_ IHXTCPResponse* pResponse);
    STDMETHOD(Bind)                 (THIS_ UINT32 uAddr, UINT16 uPort);
    STDMETHOD(Connect)              (THIS_ const char* pHost, UINT16 uPort);
    STDMETHOD(Read)                 (THIS_ UINT16 uLen);
    STDMETHOD(Write)                (THIS_ IHXBuffer* pBuf);
    STDMETHOD(WantWrite)            (THIS);
    STDMETHOD(GetForeignAddress)    (THIS_ REF(ULONG32) uAddr);
    STDMETHOD(GetLocalAddress)      (THIS_ REF(ULONG32) uAddr);
    STDMETHOD(GetForeignPort)       (THIS_ REF(UINT16)  uPort);
    STDMETHOD(GetLocalPort)         (THIS_ REF(UINT16)  uPort);

    // IHXResolveResponse
    STDMETHOD(GetAddrInfoDone)      (THIS_ HX_RESULT status,
                                           UINT32 nVecLen,
                                           IHXSockAddr** ppAddrVec);
    STDMETHOD(GetNameInfoDone)      (THIS_ HX_RESULT status,
                                           const char* pNode,
                                           const char* pService);

    // IHXWouldBlock
    STDMETHOD(WantWouldBlock)       (THIS_ IHXWouldBlockResponse*, UINT32 id);

    // IHXSetSocketOption
    STDMETHOD(SetOption)            (THIS_ HX_SOCKET_OPTION opt, UINT32 uVal);

    // IHXSocketResponse
    STDMETHOD(EventPending)         (THIS_ UINT32 uEvent, HX_RESULT status);

private:
    void DoConnect(HX_RESULT status);
    void DoRead(void);
    void DoWrite(void);
    void DoClose(void);

protected:
    INT32                       m_nRefCount;
    IHXCommonClassFactory*      m_pCCF;
    IHXNetServices*             m_pNetSvc;
    IHXTCPResponse*             m_pResponse;
    IHXResolve*                 m_pResolve;
    HXBOOL                        m_bWantWrite;
    IHXWouldBlockResponse*      m_pWouldBlockResponse;
    UINT32                      m_uWouldBlockId;
    IHXSocket*                  m_pSock;
    UINT32                      m_uEvents;
    UINT32                      m_uReadSize;
    IHXBuffer*                  m_pReadBuf;
};

class CHXOldUDPSocket : public IHXUDPSocket,
                        public IHXSetSocketOption,
                        public IHXSocketResponse
{
private:    // Unimplemented
    CHXOldUDPSocket(const CHXOldUDPSocket&);
    CHXOldUDPSocket& operator=(const CHXOldUDPSocket&);

public:
    CHXOldUDPSocket(IHXNetServices* pNetSvc);
    CHXOldUDPSocket(IHXNetServices* pNetSvc, IHXSocket* pSock);
    virtual ~CHXOldUDPSocket(void);

    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXUDPSocket
    STDMETHOD(Init)                 (THIS_ ULONG32 uAddr, UINT16 uPort,
                                           IHXUDPResponse* pResponse);
    STDMETHOD(Bind)                 (THIS_ UINT32 uAddr, UINT16 uPort);
    STDMETHOD(Read)                 (THIS_ UINT16 uLen);
    STDMETHOD(Write)                (THIS_ IHXBuffer* pBuf);
    STDMETHOD(WriteTo)              (THIS_ ULONG32 uAddr, UINT16 uPort,
                                           IHXBuffer* pBuf);
    STDMETHOD(GetLocalPort)         (THIS_ REF(UINT16) uPort);
    STDMETHOD(JoinMulticastGroup)   (THIS_ ULONG32 uAddr, ULONG32 uIfAddr);
    STDMETHOD(LeaveMulticastGroup)  (THIS_ ULONG32 uAddr, ULONG32 uIfAddr);

    // IHXSetSocketOption
    STDMETHOD(SetOption)            (THIS_ HX_SOCKET_OPTION opt, UINT32 uVal);

    // IHXSocketResponse
    STDMETHOD(EventPending)         (THIS_ UINT32 uEvent, HX_RESULT status);

private:
    void DoRead(void);
    void DoWrite(void);
    void DoClose(void);

protected:
    INT32                       m_nRefCount;
    IHXNetServices*             m_pNetSvc;
    IHXUDPResponse*             m_pResponse;
    IHXSocket*                  m_pSock;
    UINT32                      m_uEvents;
    UINT32                      m_uReadSize;
};

/*
 * More interfaces from hxengin.h that need to be examined:
 * IHXTCPSecureSocket
 * IHXSSL
 * IHXBufferedSocket
 * IHXInterruptSafe
 * IHXAsyncIOSelection
 * IHXUDPMulticastInit
 * IHXInterruptState
 * IHXLoadBalancedListen
 * IHXOverrideDefaultServices
 * IHXSetSocketOption
 * IHXThreadSafeMethods
 * IHXFastPathNetWrite
 * IHXWouldBlock
 * IHXSharedUDPServices
 * IHXSetPrivateSocketOption
 * IHXNetInterfaces
 * IHXNetworkInterfaceEnumerator
 * IHXUDPConnectedSocket
 * IHXLBoundTCPSocket
 * ...
 */

#endif /* ndef _OLDNETAPI_H */
