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

#ifndef SOCKADDRIMP_H__
#define SOCKADDRIMP_H__

#include "nettypes.h"
#include "hxengin.h"
#include "unkimp.h"


class CHXSockAddrLocal 
    : public CUnknownIMP
    , public IHXSockAddrNative
    , public IHXSockAddr
    , public IHXSockAddrLocal
{
// IUnknown
    DECLARE_UNKNOWN_NOCREATE(CHXSockAddrLocal)
private:    // Unimplemented
    CHXSockAddrLocal(const CHXSockAddrLocal&);
    CHXSockAddrLocal& operator=(CHXSockAddrLocal&);

public:
    CHXSockAddrLocal(IUnknown* pContext);
    CHXSockAddrLocal(IUnknown* pContext, const sockaddr_un* psa);

    ~CHXSockAddrLocal(void);

    STDMETHOD_(void,Get)                    (THIS_ sockaddr** ppsa,
                                                   size_t* psalen);

    STDMETHOD_(HXSockFamily,GetFamily)      (void);
    STDMETHOD_(HXBOOL,IsEqual)                (THIS_ IHXSockAddr* pOther);
    STDMETHOD(Copy)                         (THIS_ IHXSockAddr* pOther);
    STDMETHOD(Clone)                        (THIS_ IHXSockAddr** ppNew);
    STDMETHOD(GetAddr)                      (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(SetAddr)                      (THIS_ IHXBuffer* pBuf);
    STDMETHOD_(HXBOOL,IsEqualAddr)            (THIS_ IHXSockAddr* pOther);
    STDMETHOD_(UINT16, GetPort)             (THIS);
    STDMETHOD(SetPort)                      (THIS_ UINT16 port);
    STDMETHOD_(HXBOOL,IsEqualPort)            (THIS_ IHXSockAddr* pOther);
    STDMETHOD(MaskAddr)                     (THIS_ UINT32 nBits);
    STDMETHOD_(HXBOOL,IsEqualNet)             (THIS_ IHXSockAddr* pOther,
                                                   UINT32 nBits);

protected:
    sockaddr_un                 m_addr;
    IUnknown*			m_pContext;
};

class CHXSockAddrIN4 
    : public CUnknownIMP
    , public IHXSockAddrNative
    , public IHXSockAddr
    , public IHXSockAddrIN4
{
// IUnknown
    DECLARE_UNKNOWN_NOCREATE(CHXSockAddrIN4)
private:    // Unimplemented
    CHXSockAddrIN4(const CHXSockAddrIN4&);
    CHXSockAddrIN4& operator=(CHXSockAddrIN4&);

    HX_RESULT GetNativeAddr(IHXSockAddr* pAddr,
                            sockaddr_storage* pss,
                            sockaddr_in** psa);

public:
    CHXSockAddrIN4(IUnknown* pContext);
    CHXSockAddrIN4(IUnknown* pContext, const sockaddr_in* psa);
    ~CHXSockAddrIN4(void);

    STDMETHOD_(void,Get)                    (THIS_ sockaddr** ppsa,
                                                   size_t* psalen);

    STDMETHOD_(HXSockFamily,GetFamily)      (void);
    STDMETHOD_(HXBOOL,IsEqual)                (THIS_ IHXSockAddr* pOther);
    STDMETHOD(Copy)                         (THIS_ IHXSockAddr* pOther);
    STDMETHOD(Clone)                        (THIS_ IHXSockAddr** ppNew);
    STDMETHOD(GetAddr)                      (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(SetAddr)                      (THIS_ IHXBuffer* pBuf);
    STDMETHOD_(HXBOOL,IsEqualAddr)            (THIS_ IHXSockAddr* pOther);
    STDMETHOD_(UINT16, GetPort)             (THIS);
    STDMETHOD(SetPort)                      (THIS_ UINT16 port);
    STDMETHOD_(HXBOOL,IsEqualPort)            (THIS_ IHXSockAddr* pOther);
    STDMETHOD(MaskAddr)                     (THIS_ UINT32 nBits);
    STDMETHOD_(HXBOOL,IsEqualNet)             (THIS_ IHXSockAddr* pOther,
                                                   UINT32 nBits);

    STDMETHOD_(HXIN4AddrClass,GetAddrClass) (THIS);

protected:
    sockaddr_in                 m_addr;
    IUnknown*			m_pContext;
};

class CHXSockAddrIN6 
    : public CUnknownIMP
    , public IHXSockAddrNative
    , public IHXSockAddr
    , public IHXSockAddrIN6
{
// IUnknown
    DECLARE_UNKNOWN_NOCREATE(CHXSockAddrIN6)
private:    // Unimplemented
    CHXSockAddrIN6(const CHXSockAddrIN6&);
    CHXSockAddrIN6& operator=(CHXSockAddrIN6&);

    HX_RESULT GetNativeAddr(IHXSockAddr* pAddr,
                            sockaddr_storage* pss,
                            sockaddr_in6** psa);

    inline HX_RESULT SetScopeIDFromBuffer(const char* pScopeId);

public:
    CHXSockAddrIN6(IUnknown* pContext);
    CHXSockAddrIN6(IUnknown* pContext, const sockaddr_in6* psa);
    ~CHXSockAddrIN6(void);

    STDMETHOD_(void,Get)                    (THIS_ sockaddr** ppsa,
                                                   size_t* psalen);

    STDMETHOD_(HXSockFamily,GetFamily)      (void);
    STDMETHOD_(HXBOOL,IsEqual)                (THIS_ IHXSockAddr* pOther);
    STDMETHOD(Copy)                         (THIS_ IHXSockAddr* pOther);
    STDMETHOD(Clone)                        (THIS_ IHXSockAddr** ppNew);
    STDMETHOD(GetAddr)                      (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(SetAddr)                      (THIS_ IHXBuffer* pBuf);
    STDMETHOD_(HXBOOL,IsEqualAddr)            (THIS_ IHXSockAddr* pOther);
    STDMETHOD_(UINT16, GetPort)             (THIS);
    STDMETHOD(SetPort)                      (THIS_ UINT16 port);
    STDMETHOD_(HXBOOL,IsEqualPort)            (THIS_ IHXSockAddr* pOther);
    STDMETHOD(MaskAddr)                     (THIS_ UINT32 nBits);
    STDMETHOD_(HXBOOL,IsEqualNet)             (THIS_ IHXSockAddr* pOther,
                                                   UINT32 nBits);

    STDMETHOD(GetFullAddr)                  (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(GetFullAddrZ)                 (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(ExtractIN4Addr)               (THIS_ IHXSockAddrIN4** ppAddr);
    STDMETHOD_(HXIN6AddrClass,GetAddrClass) (THIS);
    STDMETHOD_(UINT32,GetFlowInfo)          (THIS);
    STDMETHOD(SetFlowInfo)                  (THIS_ UINT32 uFlowInfo);
    STDMETHOD_(UINT32,GetScopeId)           (THIS);
    STDMETHOD(SetScopeId)                   (THIS_ UINT32 uScopeId);

protected:
    sockaddr_in6             m_addr;
    IUnknown*		     m_pContext;
};



#if(0)
class CHXAddrInfo 
    : public CUnknownIMP
    , public IHXAddrInfo
{
// IUnknown
    DECLARE_UNKNOWN_NOCREATE(CHXAddrInfo)
private:    // Unimplemented
    CHXAddrInfo(const CHXAddrInfo&);
    CHXAddrInfo& operator=(const CHXAddrInfo&);

public:
    CHXAddrInfo(void);

    STDMETHOD_(UINT32,GetFlags)     (THIS);
    STDMETHOD(SetFlags)             (THIS_ UINT32 uFlags);
    STDMETHOD_(UINT32,GetFamily)    (THIS);
    STDMETHOD(SetFamily)            (THIS_ UINT32 uFamily);
    STDMETHOD_(UINT32,GetType)      (THIS);
    STDMETHOD(SetType)              (THIS_ UINT32 uType);
    STDMETHOD_(UINT32,GetProtocol)  (THIS);
    STDMETHOD(SetProtocol)          (THIS_ UINT32 uProtocol);

};
#endif


#endif /* ndef SOCKADDRIMP_H__ */
