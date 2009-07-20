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

#include "hxcom.h"
#include "hxnet.h"

#include "hxassert.h"
#include "hxbuffer.h"
#include "hxsymbiansockaddr.h"

#include <netinet/in.h>

static HX_RESULT
DoMaskAddr(TInetAddr &addr, UINT32 nBits)
{
    HX_RESULT hxr = HXR_OK;
    if(addr.Family() == KAfInet)
    {
        if(nBits <= 31)
        {
            UINT32 mask = ~((1UL << (32-nBits)) - 1);
            addr.SetAddress(addr.Address() & mask);
        }
        else if(nBits == 32)
        {
            // nothing needs to be done.
        }
        else
        {
            hxr = HXR_FAILED;
        }
    }
    else
    {
        if(nBits <= 128)
        {
            const TInetAddr tmpAddr = addr;
            addr.Prefix(tmpAddr, nBits);
        }
        else
            hxr = HXR_FAILED;
    }
    return hxr;
}

CHXInetSockAddr::CHXInetSockAddr(IUnknown *pContext):
    m_nRefCount(0)
    ,m_pContext(pContext)
    ,m_pCommonClassFactory(NULL)
{
    HX_ASSERT(m_pContext);
    HX_ADDREF(m_pContext);
    m_pContext->QueryInterface(IID_IHXCommonClassFactory,
    (void**)&m_pCommonClassFactory);
    HX_ASSERT(m_pCommonClassFactory);
    m_addr.SetFamily(KAfInet);
    m_addr.SetAddress(0);
}

CHXInetSockAddr::~CHXInetSockAddr(void)
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pCommonClassFactory);
}

STDMETHODIMP_(HXSockFamily)
CHXInetSockAddr::GetFamily(void)
{
    return m_addr.Family() == KAfInet 
           ? HX_SOCK_FAMILY_IN4
           : HX_SOCK_FAMILY_IN6;
}

STDMETHODIMP_(HXBOOL)
CHXInetSockAddr::IsEqual(IHXSockAddr* pOther)
{
    CHXInetSockAddr *pNet = (CHXInetSockAddr *) pOther;
    TInetAddr addr;
    pNet->GetNative(addr);
    return m_addr.CmpAddr(addr);
}

/* 
returns a null terminatd buffer. format of the buffer is either 
"d.d.d.d" or "h:h:h:h:h:h:h:h". Where "d" is an 8-bit decimal number 
and "h" is a 16-bit hexadecimal number depending on address family.
*/

STDMETHODIMP
CHXInetSockAddr::GetAddr(IHXBuffer** ppBuf)
{
    HX_RESULT res = HXR_OUTOFMEMORY;
    TBuf<64> addr;
    TBuf8<64> addr8;

    m_addr.Output(addr);
    addr8.Copy(addr);

    // +1 for appending '\0' 
    TInt len = addr8.Length() + 1;
    m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)ppBuf);

    if(*ppBuf)
    {
        res = (*ppBuf)->Set(addr8.PtrZ(), len);
        if(res != HXR_OK)
        {
            HX_RELEASE(*ppBuf);
        }
    }

    return res;
}

STDMETHODIMP
CHXInetSockAddr::SetAddr(IHXBuffer* pBuf)
{
    HX_ASSERT(m_nRefCount == 1);
    HX_ASSERT(pBuf != NULL && pBuf->GetBuffer() != NULL);
    TPtrC8 addr8(pBuf->GetBuffer(), pBuf->GetSize()-1);
    TBuf<64> addr;

    addr.Copy(addr8);
    if(m_addr.Input(addr) == KErrNone)
    {
        return HXR_OK;
    }
    else
    {
        return HXR_FAIL;
    }
}

STDMETHODIMP_(HXBOOL)
CHXInetSockAddr::IsEqualAddr(IHXSockAddr* pOther)
{
    CHXInetSockAddr *pNet = (CHXInetSockAddr *) pOther;
    TInetAddr addr;
    pNet->GetNative(addr);
    return m_addr.Match(addr);
}

STDMETHODIMP_(UINT16)
CHXInetSockAddr::GetPort(void)
{
    return m_addr.Port();
}


STDMETHODIMP
CHXInetSockAddr::SetPort(UINT16 port)
{
    m_addr.SetPort(port);
    return HXR_OK;
}

STDMETHODIMP_(HXBOOL)
CHXInetSockAddr::IsEqualPort(IHXSockAddr* pOther)
{
    CHXInetSockAddr *pNet = (CHXInetSockAddr *) pOther;
    TInetAddr addr;
    pNet->GetNative(addr);
    return m_addr.CmpPort(addr);
}

/*
example
192.168.1.100, nBits=24
result=192.168.1.0
*/

STDMETHODIMP
CHXInetSockAddr::MaskAddr(UINT32 nBits)
{
    return DoMaskAddr(m_addr, nBits);
}

STDMETHODIMP_(HXBOOL)
CHXInetSockAddr::IsEqualNet(IHXSockAddr* pOther, UINT32 nBits)
{
    CHXInetSockAddr *pNet = (CHXInetSockAddr *) pOther;
    TInetAddr addr1, addr2;
    pNet->GetNative(addr1);
    DoMaskAddr(addr1, nBits);
    addr2 = m_addr;
    DoMaskAddr(addr2, nBits);   
    return addr1.Match(addr2);
}

void
CHXInetSockAddr::SetNative(const TInetAddr &addr)
{
    m_addr = addr;
}

void
CHXInetSockAddr::GetNative(TInetAddr &addr)
{
    addr = m_addr;
}

STDMETHODIMP
CHXInetSockAddr::Copy(IHXSockAddr* pOther)
{
    CHXInetSockAddr* pNet = (CHXInetSockAddr*)pOther;
    pNet->SetNative(m_addr);
    return HXR_OK;
}

CHXSockAddrIN4::CHXSockAddrIN4(IUnknown *pContext):
    CHXInetSockAddr(pContext)
{
}

CHXSockAddrIN4::~CHXSockAddrIN4()
{
}

STDMETHODIMP
CHXSockAddrIN4::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXSockAddr*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSockAddr))
    {
        AddRef();
        *ppvObj = (IHXSockAddr*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSockAddrIN4))
    {
        AddRef();
        *ppvObj = (IHXSockAddrIN4*)this;
        return HXR_OK;
    }
    // added IID_IHXSockAddrNative
    if (IsEqualIID(riid, IID_IHXSockAddrNative))
    {
        AddRef();
        *ppvObj = (IHXSockAddrNative*)this;
        return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXSockAddrIN4::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXSockAddrIN4::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP_(HXIN4AddrClass)
CHXSockAddrIN4::GetAddrClass(void)
{
    HXIN4AddrClass clas = HX_IN4_CLASS_NONE; 
    TUint32 addr;

    addr = m_addr.Address();

    if (m_addr.IsLoopback())
    {
        clas = HX_IN4_CLASS_LOOPBACK;
    }
    else if (m_addr.IsMulticast())
    {
        clas = HX_IN4_CLASS_MULTICAST;
    }
    else if (addr == INADDR_BROADCAST)
    {
        clas = HX_IN4_CLASS_BROADCAST;
    }
    else if(addr == INADDR_ANY)
    {
        clas = HX_IN4_CLASS_ANY;
    }
    else if ((addr & 0xff000000) == 0x0a000000 ||
        (addr & 0xfff00000) == 0xac100000 ||
        (addr & 0xffff0000) == 0xc0a80000)
    {
        // private addresses as per RFC 1918
        clas = HX_IN4_CLASS_PRIVATE;
    }
    return clas;
}

STDMETHODIMP_(void)
CHXSockAddrIN4::Get(THIS_ sockaddr** ppsa, size_t* psalen)
{
    m_nativeAddr.sin_family = AF_INET;
    m_nativeAddr.sin_port = m_addr.Port();
    m_nativeAddr.sin_addr.s_addr = m_addr.Address();
    *ppsa = (sockaddr*)&m_nativeAddr;
    *psalen = sizeof(m_nativeAddr);
}

STDMETHODIMP
CHXSockAddrIN4::Clone(IHXSockAddr** ppNew)
{
    HX_RESULT res = HXR_OK;
    CHXSockAddrIN4 *pNew = new CHXSockAddrIN4(m_pContext);
    if (pNew == NULL)
    {
        *ppNew = NULL;
        res = HXR_OUTOFMEMORY;
    }
    else
    {
        pNew->SetNative(m_addr);
        pNew->QueryInterface(IID_IHXSockAddr, (void**)ppNew);
    }
    return res;
}

CHXSockAddrIN6::CHXSockAddrIN6(IUnknown *pContext):
    CHXInetSockAddr(pContext)
{
}

CHXSockAddrIN6::~CHXSockAddrIN6(void)
{
}

STDMETHODIMP
CHXSockAddrIN6::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXSockAddr*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSockAddr))
    {
        AddRef();
        *ppvObj = (IHXSockAddr*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSockAddrIN6))
    {
        AddRef();
        *ppvObj = (IHXSockAddrIN6*)this;
        return HXR_OK;
    }
    // added IID_IHXSockAddrNative
    if (IsEqualIID(riid, IID_IHXSockAddrNative))
    {
        AddRef();
        *ppvObj = (IHXSockAddrNative*)this;
        return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CHXSockAddrIN6::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}


STDMETHODIMP_(ULONG32)
CHXSockAddrIN6::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP_(void) 
CHXSockAddrIN6::Get(THIS_ sockaddr** ppsa, size_t* psalen)
{
    m_nativeAddr.sin6_family = AF_INET6;
    m_nativeAddr.sin6_port = m_addr.Port();

    //supposed to be already in network order
    UINT8* addr6 =  (UINT8*)&m_addr.Ip6Address().u.iAddr8;
    UINT8* p6addr = (UINT8*)&m_nativeAddr.sin6_addr.s6_addr;

    p6addr = addr6;
    *ppsa = (sockaddr*)&m_nativeAddr;
    *psalen = sizeof(m_nativeAddr);
}

STDMETHODIMP
CHXSockAddrIN6::Clone(IHXSockAddr** ppNew)
{
    HX_RESULT res = HXR_OK;
    CHXSockAddrIN6 *pNew = new CHXSockAddrIN6(m_pContext);
    if (pNew == NULL)
    {
        *ppNew = NULL;
        res = HXR_OUTOFMEMORY;
    }
    else
    {
        pNew->SetNative(m_addr);
        pNew->QueryInterface(IID_IHXSockAddr, (void**)ppNew);
    }
    return res;
}

STDMETHODIMP
CHXSockAddrIN6::GetFullAddr(IHXBuffer** ppBuf)
{
    // same as GetAddr
    return GetAddr(ppBuf);
}

STDMETHODIMP
CHXSockAddrIN6::GetFullAddrZ(IHXBuffer** ppBuf)
{
    // same as GetAddr
    return GetAddr(ppBuf);
}


STDMETHODIMP
CHXSockAddrIN6::ExtractIN4Addr(IHXSockAddrIN4** ppAddr)
{
    HX_RESULT res = HXR_OK;
    CHXSockAddrIN4 *pNew;
    if( !(m_addr.IsV4Compat() || m_addr.IsV4Mapped()))
    {
        *ppAddr = NULL;
        res = HXR_FAILED;
    }
    else if ( (pNew = new CHXSockAddrIN4(m_pContext)) == NULL )
    {
        *ppAddr = NULL;
        res = HXR_OUTOFMEMORY;
    }
    else
    {
        TInetAddr addr = m_addr;
        addr.ConvertToV4();
        pNew->SetNative(addr);
        pNew->QueryInterface(IID_IHXSockAddrIN4, (void**)ppAddr);
    }
    return res;
}


STDMETHODIMP_(HXIN6AddrClass)
CHXSockAddrIN6::GetAddrClass(void)
{
    HXIN6AddrClass clas = HX_IN6_CLASS_NONE; 

    if (m_addr.IsLoopback())
    {
        clas = HX_IN6_CLASS_LOOPBACK;
    }
    else if (m_addr.IsMulticast())
    {
        clas = HX_IN6_CLASS_MULTICAST;
    }
    else if (m_addr.IsUnspecified())
    {
        clas = HX_IN6_CLASS_ANY;
    }
    else if (m_addr.IsUnicast())
    {
        clas = HX_IN6_CLASS_UNICAST;
    }
    else if (m_addr.IsLinkLocal())
    {
        clas = HX_IN6_CLASS_LINKLOCAL;
    }
    else if (m_addr.IsSiteLocal())
    {
        clas = HX_IN6_CLASS_SITELOCAL;
    }
    else if (m_addr.IsV4Mapped())
    {
        clas = HX_IN6_CLASS_V4MAPPED;
    }
    else if (m_addr.IsV4Compat())
    {
        clas = HX_IN6_CLASS_V4COMPAT;
    }
    return clas;
}

STDMETHODIMP_(UINT32)
CHXSockAddrIN6::GetFlowInfo(void)
{
    return m_addr.FlowLabel(); 
}

STDMETHODIMP
CHXSockAddrIN6::SetFlowInfo(UINT32 uFlowInfo)
{
    HX_ASSERT(m_nRefCount == 1);
    m_addr.SetFlowLabel(uFlowInfo); 
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
CHXSockAddrIN6::GetScopeId(void)
{
    return m_addr.Scope();
}

STDMETHODIMP
CHXSockAddrIN6::SetScopeId(UINT32 uScopeId)
{
    HX_ASSERT(m_nRefCount == 1);
    m_addr.SetScope(uScopeId);
    return HXR_OK;
}

