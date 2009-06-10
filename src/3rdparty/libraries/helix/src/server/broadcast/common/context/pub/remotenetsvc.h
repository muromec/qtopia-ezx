/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: remotenetsvc.h,v 1.5 2005/05/09 19:31:20 skharkar Exp $
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

#ifndef _REMOTENETSVC_H_
#define _REMOTENETSVC_H_

#include "hxengin.h"

class CHXRemoteSocket;

class CRemoteSockCB : public IHXCallback
{
public:
    CRemoteSockCB(CHXRemoteSocket* pSock, UINT32 event);
    virtual ~CRemoteSockCB(void);

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(UINT32,AddRef)       (THIS);
    STDMETHOD_(UINT32,Release)      (THIS);

    STDMETHOD(Func)                 (THIS);

protected:
    INT32                       m_nRefCount;
    CHXRemoteSocket*            m_pSock;
    UINT32                      m_event;
};

class CHXRemoteSocket : public CHXSocket
{
public:
    CHXRemoteSocket(CHXNetServices* pNetSvc,
                    IUnknown* punkContext,
                    CallbackContainer* pCallbacks);
    CHXRemoteSocket(HXSockFamily f,
                    HXSockType t,
                    HXSockProtocol p,
                    HX_SOCK sock,
                    CHXNetServices* pNetSvc,
                    IUnknown* punkContext,
                    CallbackContainer* pCallbacks);
    virtual ~CHXRemoteSocket(void);

    virtual HX_RESULT   Select(UINT32 uEventMask, BOOL bImplicit = TRUE);
    virtual void        OnEvent(UINT32 mask);

    // Override some methods for the remote context event model
    STDMETHOD(Init)                 (THIS_ HXSockFamily f,
                                           HXSockType t,
                                           HXSockProtocol p);
    STDMETHOD(Close)                (THIS);
#ifdef _WINDOWS
    STDMETHOD (ConnectToOne)(IHXSockAddr* pAddr)
    {
	m_bAttemptingConnect = TRUE;
	return CHXSocket::ConnectToOne(pAddr);
    };

    STDMETHOD (ConnectToAny)(UINT32 nVecLen, IHXSockAddr** ppAddrVec)
    {
	m_bAttemptingConnect = TRUE;
	return CHXSocket::ConnectToAny(nVecLen,ppAddrVec);
    };
#endif
protected:
    UINT32                      m_uSelectedEventMask;
    CallbackContainer*          m_pCallbacks;
    CRemoteSockCB*              m_pCBR;
    CRemoteSockCB*              m_pCBW;
#ifdef _WINDOWS
    CRemoteSockCB*              m_pCBC;
    BOOL			m_bAttemptingConnect;
#endif
};

class CRemoteNetServicesContext : public CHXNetServices
{
public:
    CRemoteNetServicesContext(CallbackContainer* pCallback);
    virtual ~CRemoteNetServicesContext(void);

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
    CallbackContainer*          m_pCallback;
    CResolverCache*             m_pResolverCache;
};

#endif /* _REMOTENETSVC_H_ */
