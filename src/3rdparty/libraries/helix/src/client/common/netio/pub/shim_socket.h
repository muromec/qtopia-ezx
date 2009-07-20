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

#ifndef SHIM_SOCKET_H
#define SHIM_SOCKET_H

// Forward defines
_INTERFACE IHXNetServices;
_INTERFACE IHXNetworkServices;
_INTERFACE IHXSocketResponse;
_INTERFACE IHXSockAddr;
_INTERFACE IHXBuffer;
_INTERFACE IHXSocket;
_INTERFACE IHXTCPSocket;
_INTERFACE IHXScheduler;
_INTERFACE IHXUDPSocket;
class CHXGenericCallback;

// Defines
// Make sure these do not conflict with
// any of the HX_SOCK_EVENT_xxx flags in hxnet.h
#define HX_SOCKET_SHIM_FLAG_ISSUEREAD (1<<16)


class CHXClientSocketShim : public IHXSocket,
                            public IHXTCPResponse,
                            public IHXUDPResponse,
                            public IHXMulticastSocket
{
public:
    CHXClientSocketShim(IUnknown* pContext, 
        IHXNetServices* pNetServices, IHXNetworkServices* pOldNetServices);
    virtual ~CHXClientSocketShim();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXSocket methods
    STDMETHOD_(HXSockFamily,GetFamily)     (THIS) { return m_eSockFamily;   }
    STDMETHOD_(HXSockType,GetType)         (THIS) { return m_eSockType;     }
    STDMETHOD_(HXSockProtocol,GetProtocol) (THIS) { return m_eSockProtocol; }
    STDMETHOD(SetResponse)                 (THIS_ IHXSocketResponse* pResponse);
    STDMETHOD(SetAccessControl)            (THIS_ IHXSocketAccessControl* pControl);
    STDMETHOD(Init)                        (THIS_ HXSockFamily f, HXSockType t, HXSockProtocol p);
    STDMETHOD(CreateSockAddr)              (THIS_ IHXSockAddr** ppAddr);
    STDMETHOD(Bind)                        (THIS_ IHXSockAddr* pAddr);
    STDMETHOD(ConnectToOne)                (THIS_ IHXSockAddr* pAddr);
    STDMETHOD(ConnectToAny)                (THIS_ UINT32 nVecLen, IHXSockAddr** ppAddrVec);
    STDMETHOD(GetLocalAddr)                (THIS_ IHXSockAddr** ppAddr);
    STDMETHOD(GetPeerAddr)                 (THIS_ IHXSockAddr** ppAddr);
    STDMETHOD(SelectEvents)                (THIS_ UINT32 uEventMask);
    STDMETHOD(Peek)                        (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(Read)                        (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(Write)                       (THIS_ IHXBuffer* pBuf);
    STDMETHOD(Close)                       (THIS);
    STDMETHOD(Listen)                      (THIS_ UINT32 uBackLog);
    STDMETHOD(Accept)                      (THIS_ IHXSocket** ppNewSock, IHXSockAddr** ppSource);
    STDMETHOD(GetOption)                   (THIS_ HXSockOpt name, UINT32* pval);
    STDMETHOD(SetOption)                   (THIS_ HXSockOpt name, UINT32 val);
    STDMETHOD(PeekFrom)                    (THIS_ IHXBuffer** ppBuf, IHXSockAddr** ppAddr);
    STDMETHOD(ReadFrom)                    (THIS_ IHXBuffer** ppBuf, IHXSockAddr** ppAddr);
    STDMETHOD(WriteTo)                     (THIS_ IHXBuffer* pBuf, IHXSockAddr* pAddr);
    STDMETHOD(ReadV)                       (THIS_ UINT32 nVecLen, UINT32* puLenVec, IHXBuffer** ppBufVec);
    STDMETHOD(ReadFromV)                   (THIS_ UINT32 nVecLen, UINT32* puLenVec,
                                                  IHXBuffer** ppBufVec, IHXSockAddr** ppAddr);
    STDMETHOD(WriteV)                      (THIS_ UINT32 nVecLen, IHXBuffer** ppBufVec);
    STDMETHOD(WriteToV)                    (THIS_ UINT32 nVecLen, IHXBuffer** ppBufVec, IHXSockAddr* pAddr);

    // IHXMulticastSocket
    STDMETHOD(JoinGroup)            (THIS_ IHXSockAddr* pGroupAddr,
                                           IHXSockAddr* pInterface);
    STDMETHOD(LeaveGroup)           (THIS_ IHXSockAddr* pGroupAddr,

                                           IHXSockAddr* pInterface);

    STDMETHOD(SetSourceOption)      (THIS_ HXMulticastSourceOption flag,
                                           IHXSockAddr* pSourceAddr,
                                           IHXSockAddr* pGroupAddr,
                                           IHXSockAddr* pInterface);

    // IHXTCPResponse methods
    STDMETHOD(ConnectDone) (THIS_ HX_RESULT status);
    STDMETHOD(ReadDone)    (THIS_ HX_RESULT status, IHXBuffer* pBuffer);
    STDMETHOD(WriteReady)  (THIS_ HX_RESULT status);
    STDMETHOD(Closed)      (THIS_ HX_RESULT status);

    // IHXUDPResponse methods
    STDMETHOD(ReadDone) (THIS_ HX_RESULT status, IHXBuffer* pBuffer, ULONG32 ulAddr, UINT16 nPort);

    // CHXClientSocketShim methods
    HX_RESULT Init(IHXTCPSocket* pSocket);
    HX_RESULT ScheduleCallback(UINT32 ulTask);
    void      HandleCallback();
protected:
    enum SocketState
    {
        SocketStateClosed,
        SocketStateOpen,
        SocketStateConnected,
        SocketStateNamed,
        SocketStateListening,
        SocketStateConnectionPending,
        SocketStateClosePending
    };

    INT32               m_lRefCount;
    IUnknown*           m_pContext;
    HXSockFamily        m_eSockFamily;
    HXSockType          m_eSockType;
    HXSockProtocol      m_eSockProtocol;
    IHXNetServices*     m_pNetServices;
    IHXNetworkServices* m_pOldNetServices;
    IHXSocketResponse*  m_pResponse;
    IHXScheduler*       m_pScheduler;
    IHXTCPSocket*       m_pTCPSocket;
    IHXUDPSocket*       m_pUDPSocket;
    UINT32              m_ulEventMask;
    UINT32              m_ulNumSockAddr;
    UINT32              m_ulCurSockAddr;
    IHXSockAddr**       m_ppSockAddr;
    SocketState         m_eSocketState;
    IHXBuffer*          m_pLastReadBuffer;
    UINT32              m_ulLastReadOffset;
    CHXGenericCallback* m_pSocketCallback;
    UINT32              m_ulSocketTaskFlags;
    UINT32              m_ulUDPReadAddress;
    UINT16              m_usUDPReadPort;
    HXBOOL                m_bBigEndian;
    HXBOOL                m_bReadDonePending;
    IHXSockAddr*        m_pUDPConnectAddr;

    void          ClearAddressVector();
    HX_RESULT     DoConnectToOne(IHXSockAddr* pAddr);
    void          InitSocket(IUnknown* pContext, IHXNetServices* pNetServices, IHXNetworkServices* pOldNetServices);
    HX_RESULT     DoRead(IHXBuffer** ppBuffer, HXBOOL bPeek);
    HX_RESULT     DoReadFrom(IHXBuffer** ppBuffer, IHXSockAddr** ppAddr, HXBOOL bPeek);
    static void   SocketCallbackProc(void* pArg);
};

#endif /* #ifndef SHIM_SOCKET_H */
