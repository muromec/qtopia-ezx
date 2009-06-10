/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: servsockimp.h,v 1.34 2007/08/30 17:31:15 seansmith Exp $
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

#ifndef _SERVSOCKIMP_H_
#define _SERVSOCKIMP_H_

#include "hxcom.h"
#include "hxslist.h"
#include "hxengin.h"
#include "hxerror.h"
#include "hxmon.h"
#include "base_callback.h"
#include "simple_callback.h"
#include "mem_cache.h"

class INetworkAcceptor;
class ServerAccessControl;
class IHXNetworkServicesContext;
class SharedUDPPortReader;

#include "hxnet.h"
#include "netdrv.h"
#include "sockimp.h"
#include "sockaddrimp.h"

#include "access_ctrl.h"

class CHXServSocket;

class CResolverCache;

#define HX_SOCK_EVENT_DISPATCH (1<<16)
#define DEFAULT_SERV_UDP_READ_SIZE 0x1000       // 4k read buffer by default

// This is terribly inefficient, but IHXCallback passes no data :-(
class CServSockCB : public IHXCallback
{
public:
    CServSockCB(CHXServSocket* pSock, UINT32 event);
    virtual ~CServSockCB(void);

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(UINT32,AddRef)       (THIS);
    STDMETHOD_(UINT32,Release)      (THIS);

    STDMETHOD(Func)                 (THIS);

protected:
    INT32                       m_nRefCount;
    CHXServSocket*              m_pSock;
    UINT32                      m_event;
};

class CHXServSocket : public CHXSocket
{
public:
    CHXServSocket(Process* proc, BOOL bIN6);
    CHXServSocket(HXSockFamily f, HXSockType t, HXSockProtocol p,
                  HX_SOCK sock, Process* proc);
    virtual ~CHXServSocket(void);

    virtual HX_RESULT   Select(UINT32 uEventMask, BOOL bImplicit = TRUE);
    virtual void        OnEvent(UINT32 uMask);
    // Override some methods for the server's event model
    STDMETHOD(Accept)               (THIS_ IHXSocket** ppNewSock,
                                           IHXSockAddr** ppSource);

    STDMETHOD(Init)                 (THIS_ HXSockFamily f,
                                           HXSockType t,
                                           HXSockProtocol p);
    STDMETHOD(Close)                (THIS);

    // Special access for passing descriptors between procs
    void                Dispatch(int iNewProc = -1);
    sockobj_t           GetSock(void)
    {
	return m_sock.sock;
    }
    void                SetSock(sockobj_t sock)
    {
	m_sock.sock = sock;
    }
    Process*            GetSockCreationProc(void)
    {
	return m_pSockCreationProc;
    }
    Process*            GetProc(void)
    {
	return m_pProc;
    }
    void                ExitProc(void);
    void                EnterProc(Process* proc);
    BOOL                IsValid(void);
    BOOL                InDispatch(void);

protected:
    UINT32                      m_hCorePassSockCbID;
    UINT32                      m_uSelectedEventMask;
    Process*                    m_pProc;
    Process*                    m_pSockCreationProc;
    BOOL                        m_bIN6;
    CServSockCB*                m_pCBR;
    CServSockCB*                m_pCBW;
    BOOL                        m_bDontDispatch;
    BOOL                        m_bRemovedCallbacks;
};

/*
 * The listening socket is just a wrapper around an IHXSocket.
 */
class CHXServerListeningSocket : public IHXListeningSocket
{
private:    // Unimplemented
    CHXServerListeningSocket(const CHXServerListeningSocket&);
    CHXServerListeningSocket& operator=(const CHXServerListeningSocket&);

public:
    CHXServerListeningSocket(IHXNetServices* pNetServices, Process* proc);
    virtual ~CHXServerListeningSocket(void);

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

protected:
    INT32                       m_nRefCount;
    IHXListeningSocketResponse* m_pResponse;
    CHXSimpleList*              m_pSockList;
    HXSockFamily                m_family;
    HXSockType                  m_type;
    HXSockProtocol              m_protocol;
    IHXSockAddr*                m_pAddrAny;
    IHXNetServices*             m_pNetServices;
    Process*                    m_proc;
    CServerAccessControl*       m_pAccessControl;

    HX_RESULT CreateOneListeningSocket(IHXSockAddr* pAddr, IHXListeningSocket** ppSock, BOOL bIPv6Only);
};

class CServNetServices : public CHXNetServices
{
public:
    CServNetServices(Process* proc);
    virtual ~CServNetServices(void);

    HX_RESULT Init(IUnknown* punkContext);

    virtual HX_RESULT CreateSocket  (HXSockFamily f,
                                     HXSockType t,
                                     HXSockProtocol p,
                                     HX_SOCK sock,
                                     IHXSocket** ppSock);

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(UINT32,AddRef)       (THIS);
    STDMETHOD_(UINT32,Release)      (THIS);

    STDMETHOD(CreateResolver)       (THIS_ IHXResolve** ppResolver);
    STDMETHOD(CreateSockAddr)       (THIS_ HXSockFamily f,
                                           IHXSockAddr** ppAddr);
    STDMETHOD(CreateListeningSocket)(THIS_ IHXListeningSocket** ppSock);
    STDMETHOD(CreateSocket)         (THIS_ IHXSocket** ppSock);

private:
    Process*                    m_proc;
    CResolverCache*             m_pResolverCache;
};

#endif /*_SERVSOCKIMP_H_*/
