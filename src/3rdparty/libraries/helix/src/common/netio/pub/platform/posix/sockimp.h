/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sockimp.h,v 1.40 2007/07/06 20:43:59 jfinnecy Exp $ 
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
 * terms of the GNU General Public License Version 2 (the 
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

#ifndef _SOCKIMP_H
#define _SOCKIMP_H

#include "hxccf.h"
#include "nettypes.h"
#include "netdrv.h"
#include "hxengin.h"
#include "ihxcontextuser.h"
#include "hxnetdrvloader.h"
#include "hlxclib/time.h"
#include "baseobj.h"

//XXXTDM: Don't know if hxengin.h is necessary; use forward decls instead?
#include "hxengin.h"
#include "hlxclib/time.h"

class CHXWriteQueue;

class CHXSocket;

class CHXSocketConnectEnumerator : public IHXSocketResponse
{
private:    // Unimplemented
    CHXSocketConnectEnumerator(const CHXSocketConnectEnumerator&);
    CHXSocketConnectEnumerator& operator=(const CHXSocketConnectEnumerator&);

public:
    CHXSocketConnectEnumerator(CHXSocket* pSock);
    virtual ~CHXSocketConnectEnumerator(void);

    HX_RESULT Init(UINT32 nVecLen, IHXSockAddr** ppAddrVec);

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    STDMETHOD(EventPending)         (THIS_ UINT32 uEvent, HX_RESULT status);

private:
    void AttemptConnect(HX_RESULT status);
    void FinishEnumeration(HX_RESULT status);

protected:
    INT32                       m_nRefCount;
    CHXSocket*                  m_pSock;
    IHXSocketResponse*          m_pOldResponse;
    UINT32                      m_nVecLen;
    IHXSockAddr**               m_ppAddrVec;
    UINT32                      m_nIndex;
};

class CHXNetServices;

class CHXSocket : public IHXSocket,
                  public IHXMulticastSocket,
                  public IHXCallback
{
private:    // Unimplemented
    CHXSocket(const CHXSocket&);
    CHXSocket& operator=(const CHXSocket&);

public:
    CHXSocket(CHXNetServices* pNetSvc, IUnknown* pContext);
    CHXSocket(CHXNetServices* pNetSvc, IUnknown* pContext,
              HXSockFamily f, HXSockType t, HXSockProtocol p, HX_SOCK sock);
    virtual ~CHXSocket(void);

    IHXSocketResponse*  GetResponse(void);
    virtual HX_RESULT   Select(UINT32 uEventMask, HXBOOL bImplicit = TRUE) = 0;
    virtual void        OnEvent(UINT32 uEvent);
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

    STDMETHOD(Func)                 (THIS);

protected:
    HX_RESULT ErrorToStatus(int err);
    HX_RESULT GetNativeAddr(IHXSockAddr* pAddr,
                        sockaddr_storage* pss,
                        sockaddr** ppsa,
                        size_t* psalen);
    HX_RESULT SetReadBufferSize(IHXBuffer** ppBuf /*modified*/, UINT32 cbRead);
    HX_RESULT CreateReadBuffer(IHXBuffer** ppBuf);

    HX_RESULT DoClose(void);
    HX_RESULT DoInit(HXSockFamily f, HXSockType t, HXSockProtocol p);
    HX_RESULT DoRead(IHXBuffer** ppBuf, IHXSockAddr** ppAddr = NULL,
                            HXBOOL bPeek = FALSE);
    HX_RESULT DoWrite(IHXBuffer* pBuf,
                            IHXSockAddr* pAddr = NULL);
    HX_RESULT DoReadV(UINT32 nVecLen, UINT32* puLenVec, IHXBuffer** ppBufVec,
                            IHXSockAddr** ppAddr = NULL);
    HX_RESULT DoWriteV(UINT32 nVecLen, IHXBuffer** ppVec,
                            IHXSockAddr* pAddr = NULL);
    HX_RESULT HandleWriteEvent(void);

    HX_RESULT SetMulticastTTL(UINT32 ttl);
    HX_RESULT SetMulticastInfoIN4(IHXSockAddr* pGroupAddr,
                                  IHXSockAddr* pInterface,
                                  ip_mreq& req);
    HX_RESULT SetMulticastInfoIN6(IHXSockAddr* pGroupAddr,
                                  IHXSockAddr* pInterface,
                                  ipv6_mreq& req);
    HX_RESULT DoMulticastGroupOp(IHXSockAddr* pGroupAddr,IHXSockAddr* pInterface, HXBOOL bJoin);


protected:
    INT32                       m_nRefCount;
    CHXNetServices*             m_pNetSvc;
    IUnknown*                   m_punkContext;
    IHXCommonClassFactory*      m_pCCF;
    IHXScheduler*               m_pScheduler;
    HXSockFamily                m_family;
    HXSockType                  m_type;
    HXSockProtocol              m_proto;
    HXSockBufType               m_bufType;
    HXSockReadBufAlloc          m_readBufAlloc;
    HX_SOCK                     m_sock;
#if defined(MISSING_DUALSOCKET)
    HXBOOL                        m_bV6ONLY;
    CHXSocket*                  m_pSock6;       // Master socket, set on slave
    CHXSocket*                  m_pSock4;       // Slave socket, set on master
#endif
    UINT32                      m_uUserEventMask;
    UINT32                      m_uAllowedEventMask;
    UINT32                      m_uForcedEventMask;
    UINT32                      m_uPendingEventMask;
    UINT32                      m_mss;
    HXBOOL                        m_bBlocked;
    UINT32                      m_uAggLimit;
    CHXWriteQueue*              m_pWriteQueue;
    CallbackHandle              m_hIdleCallback;
    UINT32                      m_uIdleTimeout;
    time_t                      m_tLastPacket;
    IHXSocketResponse*          m_pResponse;
    IHXSocketAccessControl*     m_pAccessControl;
    CHXSocketConnectEnumerator* m_pEnumerator;
    HXNetDrvLoader              m_netDrvLoader;
    UINT32*                     m_pTotalNetReaders;
    UINT32*                     m_pTotalNetWriters;
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
    IHXListeningSocketResponse* m_pResponse;
    IHXSocket*                  m_pSock;
    HXNetDrvLoader              m_netDrvLoader;
};


/*
 * The top-level implementation is responsible for implementing IHXNetServices
 * using a subclass of CHXNetServices.  It must supply implementations for the
 * CreateResolver() method and the two CreateSocket() methods.
 */
class CHXNetServices : public IHXNetServices
		     , public IHXNetServices2
		     , public IHXContextUser
		     , public CHXBaseCountingObject
{
private:    // Unimplemented
    CHXNetServices(const CHXNetServices&);
    CHXNetServices& operator=(const CHXNetServices&);

protected:
    CHXNetServices(void);
    virtual ~CHXNetServices(void);

public:
    virtual HX_RESULT Init(IUnknown* punkContext);

    // This is used by CHXSocket::Accept().
    virtual HX_RESULT CreateSocket  (HXSockFamily f,
                                     HXSockType t,
                                     HXSockProtocol p,
                                     HX_SOCK sock,
                                     IHXSocket** ppSock) = 0;

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    STDMETHOD(CreateResolver)       (THIS_ IHXResolve** ppResolver) PURE;
    STDMETHOD(CreateSockAddr)       (THIS_ HXSockFamily f,
                                           IHXSockAddr** ppAddr);
    STDMETHOD(CreateListeningSocket)(THIS_ IHXListeningSocket** ppSock);
    STDMETHOD(CreateSocket)         (THIS_ IHXSocket** ppSock) PURE;

    // IHXNetServices2 methods
    STDMETHOD(Close)		    (THIS);

    // IHXContextUser
    STDMETHOD (RegisterContext)	    (THIS_  IUnknown* pIContext);

protected:
    INT32                       m_nRefCount;
    IUnknown*                   m_punkContext;
    HXBOOL			m_bIN6;
};

#endif /* ndef _SOCKIMP_H */
