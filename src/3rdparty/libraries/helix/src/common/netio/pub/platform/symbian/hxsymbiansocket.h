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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

#ifndef _SYMBIAN_SOCKET_H_
#define _SYMBIAN_SOCKET_H_

#include "hxnet.h"
#include "hxcom.h"
#include "hxengin.h"
#include "hxbuffer.h"

#include <es_sock.h>
#include <in_sock.h>

#include "ihxaccesspoint.h"
#include "hxapconresp.h"
#include "hxsymbianapman.h"

// forward declarations
class CHXSymbianSocket;

class CHXSymbianSocketConnector : public CActive
{
public:
    CHXSymbianSocketConnector(CHXSymbianSocket* pParent);
    ~CHXSymbianSocketConnector();
    
    void Connect(RSocket& socket, TInetAddr& inetAddr);

private:
    void            RunL();
    void            DoCancel();

    CHXSymbianSocket*      m_pParent;
    TInetAddr       m_addr;
};

class CHXSymbianSocketReader : public CActive
{
public:
    CHXSymbianSocketReader(CHXSymbianSocket* pParent);
    ~CHXSymbianSocketReader();

    HX_RESULT Read(RSocket& socket, IHXBuffer *pBuf, TInetAddr *inetAdr);

private:
    void                RunL();
    void                DoCancel();

    CHXSymbianSocket*   m_pParent;
    TPtr8               m_bufDes;
    TSockXfrLength      m_amountRead;
    TInetAddr           m_readFrom;
    IHXBuffer*          m_pBuffer;
};


class CHXSymbianSocketWriter : public CActive
{
public:
    CHXSymbianSocketWriter(CHXSymbianSocket* pParent);
    ~CHXSymbianSocketWriter();

    void Write(RSocket& socket, IHXBuffer* pBuffer, TInetAddr *dest);

private:
    void                RunL();
    void                DoCancel();

    CHXSymbianSocket*   m_pParent;
    IHXBuffer*          m_pBuffer;
    TPtr8               m_bufDes;
    TInetAddr           m_sendAddr;
};

// CSocketWriteReq holds a Write Request.
class CSocketWriteReq : public CBase
{
public:
    CSocketWriteReq();
    ~CSocketWriteReq();
    static CSocketWriteReq* Construct(IHXSockAddr *pAddr, IHXBuffer *pBuf);
    void                    GetRequest(IHXSockAddr *&pAddr, IHXBuffer *&pBuf);
    void                    SetRequest(IHXSockAddr *pAddr, IHXBuffer *pBuf);
protected:
    IHXSockAddr *m_pDest;
    IHXBuffer   *m_pBuffer;
};

class CHXSymbianSocket : public IHXSocket
                  ,public IHXMulticastSocket
{
private:    
    // Unimplemented
    CHXSymbianSocket(const CHXSymbianSocket&);
    CHXSymbianSocket& operator=(const CHXSymbianSocket&);

public:
    friend class CHXSymbianSocketConnector;
    friend class CHXSymbianSocketWriter;
    friend class CHXSymbianSocketReader;


    CHXSymbianSocket(CHXNetServices* pNetSvc, IUnknown* pContext);
    virtual ~CHXSymbianSocket(void);

    IHXSocketResponse*  GetResponse(void);
    void                ConnectEnumeratorDone(void);

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    STDMETHOD_(HXSockFamily,GetFamily)      (THIS);
    STDMETHOD_(HXSockType,GetType)          (THIS);
    STDMETHOD_(HXSockProtocol,GetProtocol)  (THIS);

    STDMETHOD(SetResponse)          (THIS_ IHXSocketResponse* pResponse);
    STDMETHOD(SetAccessControl)     (THIS_ IHXSocketAccessControl* pControl);
    STDMETHOD(Init)                 (THIS_ HXSockFamily f, HXSockType t, HXSockProtocol p);
    STDMETHOD(CreateSockAddr)       (THIS_ IHXSockAddr** ppAddr);

    STDMETHOD(Bind)                 (THIS_ IHXSockAddr* pAddr);
    STDMETHOD(ConnectToOne)         (THIS_ IHXSockAddr* pAddr);
    STDMETHOD(ConnectToAny)         (THIS_ UINT32 nVecLen, IHXSockAddr** ppAddrVec);
    STDMETHOD(GetLocalAddr)         (THIS_ IHXSockAddr** ppAddr);
    STDMETHOD(GetPeerAddr)          (THIS_ IHXSockAddr** ppAddr);

    STDMETHOD(SelectEvents)         (THIS_ UINT32 uEventMask);
    STDMETHOD(Peek)                 (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(Read)                 (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(Write)                (THIS_ IHXBuffer* pBuf);
    STDMETHOD(Close)                (THIS);

    STDMETHOD(Listen)               (THIS_ UINT32 uBackLog);
    STDMETHOD(Accept)               (THIS_ IHXSocket** ppNewSock,
                                           IHXSockAddr** ppSource);

    STDMETHOD(GetOption)            (THIS_ HXSockOpt name, UINT32* pval);
    STDMETHOD(SetOption)            (THIS_ HXSockOpt name, UINT32 val);

    STDMETHOD(PeekFrom)             (THIS_ IHXBuffer** ppBuf,
                                        IHXSockAddr** ppAddr);
    STDMETHOD(ReadFrom)             (THIS_ IHXBuffer** ppBuf,
                                        IHXSockAddr** ppAddr);
    // Write to multiple destinations not supported.
    STDMETHOD(WriteTo)              (THIS_ IHXBuffer* pBuf, IHXSockAddr* pAddr);

    STDMETHOD(ReadV)                (THIS_ UINT32 nVecLen, UINT32* puLenVec,
                                        IHXBuffer** ppVec);
    STDMETHOD(ReadFromV)            (THIS_ UINT32 nVecLen, UINT32* puLenVec,
                                        IHXBuffer** ppVec,
                                        IHXSockAddr** ppAddr);

    STDMETHOD(WriteV)               (THIS_ UINT32 nVecLen,
                                        IHXBuffer** ppBufVec);
    STDMETHOD(WriteToV)             (THIS_ UINT32 nVecLen,
                                        IHXBuffer** ppBufVec,
                                        IHXSockAddr* pAddr);

    STDMETHOD(JoinGroup)            (THIS_ IHXSockAddr* pGroupAddr,
                                        IHXSockAddr* pInterface);
    STDMETHOD(LeaveGroup)           (THIS_ IHXSockAddr* pGroupAddr,
                                        IHXSockAddr* pInterface);
    STDMETHOD(SetSourceOption)      (THIS_ HXMulticastSourceOption flag,
                                        IHXSockAddr* pSourceAddr,
                                        IHXSockAddr* pGroupAddr,
                                        IHXSockAddr* pInterface);

    void                            GetNativeAddr(IHXSockAddr* pAddr, 
                                                  TInetAddr &inetAddr);
    void                            SetNativeAddr(IHXSockAddr* pAddr, 
                                                  TInetAddr &inetAddr);

    static void                     static_APConnectDone(void* pObj, 
                                                HX_RESULT status);

protected:
    // callbacks for async ops
    void      OnConnect(HX_RESULT hxr);
    HX_RESULT OnBind(HX_RESULT hxr);
    void      OnWrite(HX_RESULT status);
    void      OnRead(HX_RESULT status, UINT32 bytes);
    HX_RESULT OnAccessPointConnect(HX_RESULT status);

    HX_RESULT           DoClose(HX_RESULT hxr);
    HX_RESULT DoInit(HXSockFamily f, HXSockType t, HXSockProtocol p);
    HX_RESULT DoRead(IHXBuffer** ppBuf, HXBOOL bPeek = FALSE);
    HX_RESULT DoReadFrom(IHXBuffer** ppBuf, IHXSockAddr** ppAddr,
                        HXBOOL bPeek = FALSE);
    HX_RESULT DoWrite(IHXBuffer* pBuf,
                        IHXSockAddr* pAddr = NULL);
    HX_RESULT DoReadV(UINT32 nVecLen, UINT32* puLenVec, IHXBuffer** ppBufVec,
                        IHXSockAddr** ppAddr = NULL);
    HX_RESULT DoWriteV(UINT32 nVecLen, IHXBuffer** ppVec,
                        IHXSockAddr* pAddr = NULL);
    HX_RESULT DoMulticastOperation(TUint operation, IHXSockAddr *pGroupAddr);
    void      SendEvents(INT32 events, HX_RESULT hxr);

    HX_RESULT CreateReadBuffer(REF(IHXBuffer *) pBuf, UINT32 size);
    HX_RESULT TransferBuffer(REF(IHXBuffer *)in, REF(IHXBuffer *)out, UINT32 bytes);


protected:
    enum SocketState
    {
        EClosed,
        EOpen,
        EConnected,
        EBindPending,
        EBound,
        EListening,
        EConnectionPending,
        EClosePending
    };

    INT32                       m_nRefCount;
    CHXNetServices*             m_pNetSvc;
    IUnknown*                   m_pContext;
    IHXCommonClassFactory*      m_pCCF;
    IHXScheduler*               m_pScheduler;
    IHXSocketResponse*          m_pResponse;
    IHXSocketAccessControl*     m_pAccessControl;
    IHXAccessPointManager*      m_pAPManager;
    HXAccessPointConnectResp*   m_pAPResponse;
    HXSockFamily                m_family;
    HXSockType                  m_type;
    HXSockProtocol              m_proto;
    HXSockBufType               m_bufType;
    HXSockReadBufAlloc          m_readBufAlloc;
    SocketState                 m_state;
    RSocket                     m_sock;
    TInetAddr                   m_peerAddr; // can be used in Connect
    TInetAddr                   m_localAddr; // can be used in Bind
    RSocketServ                 m_sockServ;
    CHXSymbianSocketConnector*  m_pConnector;
    CHXSymbianSocketWriter*     m_pWriter;
    CHXSymbianSocketReader*     m_pReader;
    TBool                       m_bConnected;
    UINT32                      m_ulSocketTaskFlags;
    UINT32                      m_ulEventMask;
    IHXBuffer*                  m_pDataBuf;    // if data is available it will be here
    IHXBuffer*                  m_pReadersBuf; // reader will read in this buffer
    UINT32                      m_uReadSize; // preferred buffer size for read.
    TInetAddr                   m_readFrom;  // address of peer in RecvFrom.
    TInetAddr                   m_writeTo;   // address of peer for write
    TInetAddr*                  m_pUdpConnectAddr; // udp socket is connected to this address
    CHXSimpleList               m_writeList; 
    HXBOOL                      m_bInitialized;  // true if socket was opened

};


/*
 * The listening socket is just a wrapper around an IHXSocket.
 */
class CHXListeningSocket : public IHXListeningSocket,
                           public IHXSocketResponse
{
private:    // Unimplemented
    CHXListeningSocket(const CHXListeningSocket&);
    CHXListeningSocket& operator=(const CHXListeningSocket&);

public:
    CHXListeningSocket(IHXSocket* pSock);
    virtual ~CHXListeningSocket(void);

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    STDMETHOD_(HXSockFamily,GetFamily)      (THIS);
    STDMETHOD_(HXSockType,GetType)          (THIS);
    STDMETHOD_(HXSockProtocol,GetProtocol)  (THIS);

    STDMETHOD(Init)                 (THIS_ HXSockFamily f, HXSockType t, HXSockProtocol p,
                                           IHXListeningSocketResponse* pResponse);
    STDMETHOD(Listen)               (THIS_ IHXSockAddr* pAddr);
    STDMETHOD(Close)                (THIS);

    STDMETHOD(GetOption)            (THIS_ HXSockOpt name, UINT32* pval);
    STDMETHOD(SetOption)            (THIS_ HXSockOpt name, UINT32 val);

    STDMETHOD(EventPending)         (THIS_ UINT32 uEvent, HX_RESULT status);

protected:
    INT32                       m_nRefCount;
    IHXSocket*                  m_pSock;
};


#endif /* ndef _SYMBIAN_SOCKET_H_ */
