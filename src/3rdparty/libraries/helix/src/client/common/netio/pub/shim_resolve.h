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

#ifndef SHIM_RESOLVE_H
#define SHIM_RESOLVE_H

// Forward defines
_INTERFACE IHXNetServices;
_INTERFACE IHXResolveResponse;
_INTERFACE IHXAddrInfo;
_INTERFACE IHXSockAddr;
_INTERFACE IHXResolver;
_INTERFACE IHXNetworkServices;

class CHXResolveShim : public IHXResolve,
                       public IHXResolverResponse
{
public:
    CHXResolveShim(IUnknown* pContext, IHXNetServices* pServices, IHXNetworkServices* pOldServices);
    virtual ~CHXResolveShim();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXResolve methods we override
    STDMETHOD(Init)        (THIS_ IHXResolveResponse* pResponse);
    STDMETHOD(Close)                (THIS);
    STDMETHOD(GetAddrInfo) (THIS_ const char* pNode, const char* pServ, IHXAddrInfo* pHints);
    STDMETHOD(GetNameInfo) (THIS_ IHXSockAddr* pAddr, UINT32 uFlags);

    // IHXResolverResponse methods
    STDMETHOD(GetHostByNameDone) (THIS_ HX_RESULT status, ULONG32 ulAddr);
protected:
    INT32               m_lRefCount;
    HXBOOL                m_bBigEndian;
    IUnknown*           m_pContext;
    IHXNetServices*     m_pServices;
    IHXNetworkServices* m_pOldServices;
    IHXResolveResponse* m_pResponse;
    IHXResolver*        m_pResolver;
    UINT16              m_usServicePort;
};

#endif /* #ifndef SHIM_RESOLVE_H */
