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

#ifndef SHIM_LISTENING_SOCKET_H
#define SHIM_LISTENING_SOCKET_H

// Forward defines
_INTERFACE IHXNetServices;
_INTERFACE IHXNetworkServices;
_INTERFACE IHXListeningSocketResponse;
_INTERFACE IHXSockAddr;
_INTERFACE IHXSocket;
_INTERFACE IHXTCPSocket;
_INTERFACE IHXListeningSocket;



class CHXListeningSocketShim : public IHXListeningSocket,
                               public IHXListenResponse
{
public:
    CHXListeningSocketShim(IUnknown* pContext, 
        IHXNetServices* pNetServices, IHXNetworkServices* pOldNetServices);
    virtual ~CHXListeningSocketShim();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXListeningSocket
    STDMETHOD_(HXSockFamily,GetFamily)     (THIS) { return m_eSockFamily;   }
    STDMETHOD_(HXSockType,GetType)         (THIS) { return m_eSockType;     }
    STDMETHOD_(HXSockProtocol,GetProtocol) (THIS) { return m_eSockProtocol; }
    STDMETHOD(Init)                        (THIS_ HXSockFamily f, HXSockType t, HXSockProtocol p,
                                                  IHXListeningSocketResponse* pResponse);
    STDMETHOD(Listen)                      (THIS_ IHXSockAddr* pAddr);
    STDMETHOD(Accept)                      (THIS_ IHXSocket** ppNewSock, IHXSockAddr** ppSource);
    STDMETHOD(Close)                       (THIS);
    STDMETHOD(GetOption)                   (THIS_ HXSockOpt name, UINT32* pval);
    STDMETHOD(SetOption)                   (THIS_ HXSockOpt name, UINT32 val);

    // IHXListenResponse methods
    STDMETHOD(NewConnection) (THIS_ HX_RESULT status, IHXTCPSocket* pTCPSocket);
protected:
    INT32                       m_lRefCount;
    HXBOOL                        m_bBigEndian;
    IUnknown*                   m_pContext;
    IHXNetServices*             m_pNetServices;
    IHXNetworkServices*         m_pOldNetServices;
    HXSockFamily                m_eSockFamily;
    HXSockType                  m_eSockType;
    HXSockProtocol              m_eSockProtocol;
    IHXListeningSocketResponse* m_pResponse;
    IHXListenSocket*            m_pListenSocket;
    IHXTCPSocket*               m_pTCPSocket;

    HX_RESULT CreateInitSocket(IHXTCPSocket* pTCPSocket, IHXSocket** ppSocket, IHXSockAddr** ppAddr);
};

#endif /* #ifndef SHIM_LISTENING_SOCKET_H */
