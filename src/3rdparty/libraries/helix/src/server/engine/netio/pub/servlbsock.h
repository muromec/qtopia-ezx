/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: servlbsock.h,v 1.2 2004/12/10 04:08:17 tmarshall Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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

#ifndef _SERVLBSOCK_H_
#define _SERVLBSOCK_H_

#include "hxcom.h"
#include "hxslist.h"
#include "hxengin.h"
#include "hxerror.h"
#include "mem_cache.h"

#include "hxnet.h"
#include "netdrv.h"

/*
 * So called "localbound" socket implementation.  This is a userspace
 * implementation of the socket interface.  It is useful in cases where
 * code expects to be communicating with an actual socket but additional
 * processing needs to done on the data (eg. cloaking).  It is also used
 * in the proxy due to historical design constraints.
 */

class CHXSockAddrLB : public IHXSockAddr
{
private:    // Unimplemented
    CHXSockAddrLB(const CHXSockAddrLB&);
    CHXSockAddrLB& operator=(CHXSockAddrLB&);

public:
    CHXSockAddrLB(void* pSock);
    // No need for a dtor
    // virtual ~CHXSockAddrLB(void);

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    STDMETHOD_(HXSockFamily,GetFamily)      (void);
    STDMETHOD_(BOOL,IsEqual)                (THIS_ IHXSockAddr* pOther);
    STDMETHOD(Copy)                         (THIS_ IHXSockAddr* pOther);
    STDMETHOD(Clone)                        (THIS_ IHXSockAddr** ppNew);
    STDMETHOD(GetAddr)                      (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(SetAddr)                      (THIS_ IHXBuffer* pBuf);
    STDMETHOD_(BOOL,IsEqualAddr)            (THIS_ IHXSockAddr* pOther);
    STDMETHOD_(UINT16, GetPort)             (THIS);
    STDMETHOD(SetPort)                      (THIS_ UINT16 port);
    STDMETHOD_(BOOL,IsEqualPort)            (THIS_ IHXSockAddr* pOther);
    STDMETHOD(MaskAddr)                     (THIS_ UINT32 nBits);
    STDMETHOD_(BOOL,IsEqualNet)             (THIS_ IHXSockAddr* pOther,
                                                   UINT32 nBits);

protected:
    INT32                       m_nRefCount;
    void*                       m_pSock;
};

class CLBSocket : public IHXSocket,
                  public IHXCallback
{
public:
    CLBSocket(Process* pProc);
    CLBSocket(Process* pProc, CLBSocket* pPeer);
    virtual ~CLBSocket(void);

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    /* Basic methods */

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
    STDMETHOD(WriteTo)              (THIS_ IHXBuffer* pBuf,
                                           IHXSockAddr* pAddr);

    STDMETHOD(ReadV)                (THIS_ UINT32 nVecLen, UINT32* puLenVec,
                                           IHXBuffer** ppBufVec);
    STDMETHOD(ReadFromV)            (THIS_ UINT32 nVecLen, UINT32* puLenVec,
                                           IHXBuffer** ppBufVec,
                                           IHXSockAddr** ppAddr);

    STDMETHOD(WriteV)               (THIS_ UINT32 nVecLen,
                                           IHXBuffer** ppBufVec);
    STDMETHOD(WriteToV)             (THIS_ UINT32 nVecLen,
                                           IHXBuffer** ppBufVec,
                                           IHXSockAddr* pAddr);

    /* IHXCallback methods */
    STDMETHOD(Func)                 (THIS);

    // Other methods
    void ScheduleReadCallback(void);
    void ScheduleWriteCallback(void);
    void ConnectComplete(void);

protected:
    UINT32              m_ulRefCount;
    UINT32              m_ulSelectedEvents;
    UINT32              m_ulAllowedEvents;
    UINT32              m_ulPendingEvents;
    CallbackHandle      m_CallbackHandle;
    CLBSocket*          m_pPeer;
    IHXScheduler*       m_pScheduler;
    IHXSocketResponse*  m_pResponse;
    Process*            m_pProc;
    UINT32              m_uMaxPacketQueueSize;
    CHXSimpleList       m_PacketQueue;
};

#endif /*_SERVLBSOCK_H_*/
