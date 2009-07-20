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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxbuffer.h"
#include "hxsockutil.h"
#include "hxinetaddr.h"
#include "hxassert.h"
#include "pckunpck.h"
#include "sockaddrimp.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif


/*** CHXSockAddrLocal ***/

// IUnknown
BEGIN_INTERFACE_LIST_NOCREATE(CHXSockAddrLocal)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXSockAddrNative)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXSockAddr)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXSockAddrLocal)
END_INTERFACE_LIST


CHXSockAddrLocal::CHXSockAddrLocal(IUnknown* pContext)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_LOCAL;
};

CHXSockAddrLocal::CHXSockAddrLocal(IUnknown* pContext, const sockaddr_un* psa)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    memcpy(&m_addr, psa, sizeof(m_addr));
}

CHXSockAddrLocal::~CHXSockAddrLocal()
{
    HX_RELEASE(m_pContext);
}

STDMETHODIMP_(void)
CHXSockAddrLocal::Get(sockaddr** ppsa, size_t* psalen)
{
    *ppsa = (sockaddr*)&m_addr;
    *psalen = sizeof(m_addr);
}

STDMETHODIMP_(HXSockFamily)
CHXSockAddrLocal::GetFamily(void)
{
    return HX_SOCK_FAMILY_LOCAL;
}

STDMETHODIMP_(HXBOOL)
CHXSockAddrLocal::IsEqual(IHXSockAddr* pOther)
{
    HXBOOL rc = FALSE;
    if (pOther->GetFamily() == HX_SOCK_FAMILY_LOCAL)
    {
        CHXSockAddrLocal* pOtherL = (CHXSockAddrLocal*)pOther;
        rc = (memcmp(&m_addr, &pOtherL->m_addr, sizeof(m_addr)) == 0);
    }
    return rc;
}

STDMETHODIMP
CHXSockAddrLocal::Copy(IHXSockAddr* pOther)
{
    HX_RESULT hxr = HXR_FAIL;
    if (pOther->GetFamily() == HX_SOCK_FAMILY_LOCAL)
    {
        CHXSockAddrLocal* pOtherL = (CHXSockAddrLocal*)pOther;
        memcpy(&pOtherL->m_addr, &m_addr, sizeof(m_addr));
        hxr = HXR_OK;
    }
    return hxr;
}

STDMETHODIMP
CHXSockAddrLocal::Clone(IHXSockAddr** ppNew)
{
    CHXSockAddrLocal* pNew = new CHXSockAddrLocal(m_pContext);
    if (pNew == NULL)
    {
        *ppNew = NULL;
        return HXR_OUTOFMEMORY;
    }
    pNew->QueryInterface(IID_IHXSockAddr, (void**)ppNew);
    memcpy(&pNew->m_addr, &m_addr, sizeof(m_addr));
    return HXR_OK;
}

STDMETHODIMP
CHXSockAddrLocal::GetAddr(IHXBuffer** ppBuf)
{
    *ppBuf = NULL;
#ifdef _WIN32
    return HXR_FAIL;
#else
    if (HXR_OK == CreateBufferCCF(*ppBuf, m_pContext))
    {
	size_t len = strlen(m_addr.sun_path)+1;
	(*ppBuf)->SetSize(len);
	memcpy((*ppBuf)->GetBuffer(), m_addr.sun_path, len);
	return HXR_OK;
    }
    else
    {
	return HXR_OUTOFMEMORY;
    }
#endif
}

STDMETHODIMP
CHXSockAddrLocal::SetAddr(IHXBuffer* pBuf)
{
    HX_ASSERT(m_lCount == 1);
    HX_ASSERT(pBuf != NULL && pBuf->GetBuffer() != NULL);
    HX_ASSERT(*(pBuf->GetBuffer()+pBuf->GetSize()-1) == 0);
#ifdef _WIN32
    return HXR_FAIL;
#else
    size_t len = pBuf->GetSize();
    if (len > sizeof(m_addr.sun_path))
    {
        return HXR_FAIL;
    }
    memcpy(m_addr.sun_path, pBuf->GetBuffer(), len);
    return HXR_OK;
#endif
}

STDMETHODIMP_(HXBOOL)
CHXSockAddrLocal::IsEqualAddr(IHXSockAddr* pOther)
{
    return IsEqual(pOther);
}

STDMETHODIMP_(UINT16)
CHXSockAddrLocal::GetPort(void)
{
    HX_ASSERT(FALSE);
    return 0;
}

STDMETHODIMP
CHXSockAddrLocal::SetPort(UINT16 port)
{
    HX_ASSERT(m_lCount == 1);
    HX_ASSERT(FALSE);
    return HXR_NOTIMPL;
}

STDMETHODIMP_(HXBOOL)
CHXSockAddrLocal::IsEqualPort(IHXSockAddr* pOther)
{
    HX_ASSERT(FALSE);
    return FALSE;
}

STDMETHODIMP
CHXSockAddrLocal::MaskAddr(UINT32 nBits)
{
    HX_ASSERT(m_lCount == 1);
    HX_ASSERT(FALSE);
    return HXR_NOTIMPL;
}

STDMETHODIMP_(HXBOOL)
CHXSockAddrLocal::IsEqualNet(IHXSockAddr* pOther, UINT32 nBits)
{
    HX_ASSERT(FALSE);
    return FALSE;
}

/*** 
    CHXSockAddrIN4 
***/

BEGIN_INTERFACE_LIST_NOCREATE(CHXSockAddrIN4)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXSockAddrNative)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXSockAddr)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXSockAddrIN4)
END_INTERFACE_LIST


CHXSockAddrIN4::CHXSockAddrIN4(IUnknown* pContext)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
}

CHXSockAddrIN4::CHXSockAddrIN4(IUnknown* pContext, const sockaddr_in* psa)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    memcpy(&m_addr, psa, sizeof(m_addr));
}

CHXSockAddrIN4::~CHXSockAddrIN4()
{
    HX_RELEASE(m_pContext);
}

STDMETHODIMP_(void)
CHXSockAddrIN4::Get(sockaddr** ppsa, size_t* psalen)
{
    *ppsa = (sockaddr*)&m_addr;
    *psalen = sizeof(m_addr);
}

STDMETHODIMP_(HXSockFamily)
CHXSockAddrIN4::GetFamily(void)
{
    return HX_SOCK_FAMILY_IN4;
}

STDMETHODIMP_(HXBOOL)
CHXSockAddrIN4::IsEqual(IHXSockAddr* pOther)
{
    HXBOOL rc = FALSE;

    sockaddr_storage ss;
    sockaddr_in* psa4;

    if (SUCCEEDED(GetNativeAddr(pOther, &ss, &psa4)))
    {
        rc = ((m_addr.sin_addr.s_addr ==psa4->sin_addr.s_addr)
           && (m_addr.sin_port == psa4->sin_port));
    }

    return rc;
}

STDMETHODIMP
CHXSockAddrIN4::Copy(IHXSockAddr* pOther)
{
    HX_RESULT hxr = HXR_FAIL;
    if (pOther->GetFamily() == HX_SOCK_FAMILY_IN4)
    {
        CHXSockAddrIN4* pOther4 = (CHXSockAddrIN4*)pOther;
        memcpy(&pOther4->m_addr, &m_addr, sizeof(m_addr));
        hxr = HXR_OK;
    }
    return hxr;
}

STDMETHODIMP
CHXSockAddrIN4::Clone(IHXSockAddr** ppNew)
{
    CHXSockAddrIN4* pNew = new CHXSockAddrIN4(m_pContext);
    if (pNew == NULL)
    {
        *ppNew = NULL;
        return HXR_OUTOFMEMORY;
    }
    pNew->QueryInterface(IID_IHXSockAddr, (void**)ppNew);
    memcpy(&pNew->m_addr, &m_addr, sizeof(m_addr));
    return HXR_OK;
}

STDMETHODIMP
CHXSockAddrIN4::GetAddr(IHXBuffer** ppBuf)
{
    char szAddr[HX_ADDRSTRLEN_IN4];
    if (hx_inet_ntop(AF_INET, &m_addr.sin_addr, szAddr, sizeof(szAddr)) == NULL)
    {
        // Should never fail
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }
    size_t len = strlen(szAddr)+1;
    if (HXR_OK == CreateBufferCCF(*ppBuf, m_pContext))
    {
	(*ppBuf)->SetSize(len);
	memcpy((*ppBuf)->GetBuffer(), szAddr, len);
	return HXR_OK;
    }
    else
    {
	return HXR_OUTOFMEMORY;
    }
}

STDMETHODIMP
CHXSockAddrIN4::SetAddr(IHXBuffer* pBuf)
{
    HX_ASSERT(m_lCount == 1);
    HX_ASSERT(pBuf != NULL && pBuf->GetBuffer() != NULL);
    HX_ASSERT(*(pBuf->GetBuffer()+pBuf->GetSize()-1) == 0);
    if (hx_inet_pton(AF_INET, (const char*)pBuf->GetBuffer(), &m_addr.sin_addr) <= 0)
    {
        return HXR_FAIL;
    }
    return HXR_OK;
}

STDMETHODIMP_(HXBOOL)
CHXSockAddrIN4::IsEqualAddr(IHXSockAddr* pOther)
{
    HXBOOL rc = FALSE;

    sockaddr_storage ss;
    sockaddr_in* psa4;

    if (SUCCEEDED(GetNativeAddr(pOther, &ss, &psa4)))
    {
        rc = (m_addr.sin_addr.s_addr == psa4->sin_addr.s_addr);
    }

    return rc;
}

STDMETHODIMP_(UINT16)
CHXSockAddrIN4::GetPort(void)
{
    return hx_ntohs(m_addr.sin_port);
}

STDMETHODIMP
CHXSockAddrIN4::SetPort(UINT16 port)
{
    HX_ASSERT(m_lCount == 1);
    m_addr.sin_port = hx_htons(port);
    return HXR_OK;
}

STDMETHODIMP_(HXBOOL)
CHXSockAddrIN4::IsEqualPort(IHXSockAddr* pOther)
{
    if ((pOther->GetFamily() != HX_SOCK_FAMILY_IN4)
    &&  (pOther->GetFamily() != HX_SOCK_FAMILY_IN6))
    {
        return FALSE;
    }

    return (GetPort() == pOther->GetPort());
}

STDMETHODIMP
CHXSockAddrIN4::MaskAddr(UINT32 nBits)
{
    HX_ASSERT(m_lCount == 1);
    if (!hx_maskaddr4(&m_addr.sin_addr, nBits))
    {
        return HXR_FAIL;
    }
    return HXR_OK;
}

STDMETHODIMP_(HXBOOL)
CHXSockAddrIN4::IsEqualNet(IHXSockAddr* pOther, UINT32 nBits)
{
    HXBOOL rc = FALSE;

    sockaddr_storage ss;
    sockaddr_in* psa4;

    if (SUCCEEDED(GetNativeAddr(pOther, &ss, &psa4)))
    {
        struct in_addr thisaddr, thataddr;
        memcpy(&thisaddr, &m_addr.sin_addr, sizeof(struct in_addr));
        memcpy(&thataddr, &psa4->sin_addr, sizeof(struct in_addr));
        if (hx_maskaddr4(&thisaddr, nBits) && hx_maskaddr4(&thataddr, nBits))
        {
            rc = (memcmp(&thisaddr, &thataddr, sizeof(struct in_addr)) == 0);
        }
    }

    return rc;
}

STDMETHODIMP_(HXIN4AddrClass)
CHXSockAddrIN4::GetAddrClass(void)
{
    in_addr_t haddr = hx_ntohl(m_addr.sin_addr.s_addr);
    if (haddr == INADDR_ANY)
    {
        return HX_IN4_CLASS_ANY;
    }
    if ((haddr >> 24) == 127)                   //XXXTDM: better way?
    {
        return HX_IN4_CLASS_LOOPBACK;
    }
    if (IN_MULTICAST(haddr))
    {
        return HX_IN4_CLASS_MULTICAST;
    }
    if (haddr == INADDR_BROADCAST)              //XXXTDM: network broadcast?
    {
        return HX_IN4_CLASS_BROADCAST;
    }
    if ((haddr & 0xff000000) == 0x0a000000 ||
        (haddr & 0xfff00000) == 0xac100000 ||
        (haddr & 0xffff0000) == 0xc0a80000)
    {
        return HX_IN4_CLASS_PRIVATE;
    }
    return HX_IN4_CLASS_NONE;
}


HX_RESULT
CHXSockAddrIN4::GetNativeAddr(IHXSockAddr* pAddr,
                              sockaddr_storage* pss,
                              sockaddr_in** ppsa)
{
    HX_ASSERT(pss);

    size_t salen; // Dummy var-- not really used for anything.
    sockaddr* psa;
    sockaddr_in* psa4;

    IHXSockAddrNative* pNative = NULL;
    HX_RESULT hxr = pAddr->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);

    if (SUCCEEDED(hxr))
    {
        pNative->Get(&psa, &salen);

        if (psa->sa_family == AF_INET6)
        {
            psa4 = (struct sockaddr_in*)pss;
            if (hx_map6to4(psa, psa4))
            {
                *ppsa = (sockaddr_in*)pss;
            }
            else
            {
                hxr = HXR_SOCK_AFNOSUPPORT;
            }
        }
        else if (psa->sa_family == AF_INET)
        {
            *ppsa = (sockaddr_in*)psa;
        }
        else
        {
            hxr = HXR_SOCK_AFNOSUPPORT;
        }

        HX_RELEASE(pNative);
    }
    return hxr;
}


/*** CHXSockAddrIN6 ***/


BEGIN_INTERFACE_LIST_NOCREATE(CHXSockAddrIN6)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXSockAddrNative)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXSockAddr)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXSockAddrIN6)
END_INTERFACE_LIST

CHXSockAddrIN6::CHXSockAddrIN6(IUnknown* pContext)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}

CHXSockAddrIN6::CHXSockAddrIN6(IUnknown* pContext, const sockaddr_in6* psa)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    memcpy(&m_addr, psa, sizeof(m_addr));
}

CHXSockAddrIN6::~CHXSockAddrIN6()
{
    HX_RELEASE(m_pContext);
}

STDMETHODIMP_(void)
CHXSockAddrIN6::Get(sockaddr** ppsa, size_t* psalen)
{
    *ppsa = (sockaddr*)&m_addr;
    *psalen = sizeof(m_addr);
}

STDMETHODIMP_(HXSockFamily)
CHXSockAddrIN6::GetFamily(void)
{
    return HX_SOCK_FAMILY_IN6;
}

STDMETHODIMP_(HXBOOL)
CHXSockAddrIN6::IsEqual(IHXSockAddr* pOther)
{
    HXBOOL rc = FALSE;

    sockaddr_storage ss;
    sockaddr_in6* psa6;

    if (SUCCEEDED(GetNativeAddr(pOther, &ss, &psa6)))
    {
        rc = IN6_ARE_ADDR_EQUAL(&m_addr.sin6_addr, &psa6->sin6_addr) &&
             m_addr.sin6_port == psa6->sin6_port;
    }

    return rc;
}

STDMETHODIMP
CHXSockAddrIN6::Copy(IHXSockAddr* pOther)
{
    HX_RESULT hxr = HXR_FAIL;
    if (pOther->GetFamily() == HX_SOCK_FAMILY_IN6)
    {
        CHXSockAddrIN6* pOther6 = (CHXSockAddrIN6*)pOther;
        memcpy(&pOther6->m_addr, &m_addr, sizeof(m_addr));
        hxr = HXR_OK;
    }
    return hxr;
}

STDMETHODIMP
CHXSockAddrIN6::Clone(IHXSockAddr** ppNew)
{
    CHXSockAddrIN6* pNew = new CHXSockAddrIN6(m_pContext);
    if (pNew == NULL)
    {
        *ppNew = NULL;
        return HXR_OUTOFMEMORY;
    }
    pNew->QueryInterface(IID_IHXSockAddr, (void**)ppNew);
    memcpy(&pNew->m_addr, &m_addr, sizeof(m_addr));
    return HXR_OK;
}

STDMETHODIMP
CHXSockAddrIN6::GetAddr(IHXBuffer** ppBuf)
{
    char szAddr[HX_ADDRSTRLEN_IN6];
    if (hx_inet_ntop(AF_INET6, &m_addr.sin6_addr, szAddr, sizeof(szAddr)) == NULL)
    {
        // Should never fail
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }
    size_t len = strlen(szAddr)+1;
    if (HXR_OK == CreateBufferCCF(*ppBuf, m_pContext))
    {
	(*ppBuf)->SetSize(len);
	memcpy((*ppBuf)->GetBuffer(), szAddr, len);
	return HXR_OK;
    }
    else
    {
	return HXR_OUTOFMEMORY;
    }
}

STDMETHODIMP
CHXSockAddrIN6::SetAddr(IHXBuffer* pBuf)
{
    HX_RESULT res = HXR_OK;
    HX_ASSERT(pBuf != NULL && pBuf->GetBuffer() != NULL);
    HX_ASSERT(*(pBuf->GetBuffer()+pBuf->GetSize()-1) == 0);

    char szBareAddr[HX_ADDRSTRLEN_IN6];
    char* pszBareAddr = NULL;
    char* pszIPV6Addr = (char*)pBuf->GetBuffer();

    char* pscopeid_delimiter = strchr(pszIPV6Addr,'%');
    if (pscopeid_delimiter)
    {
        res = SetScopeIDFromBuffer(pscopeid_delimiter+1);
        if (FAILED(res))
        {
            return res;
        }

        UINT32 ulBytesToCopy = pscopeid_delimiter-pszIPV6Addr;
        if (ulBytesToCopy >= sizeof(szBareAddr))
        {
            return HXR_INVALID_PARAMETER;
        }

        memcpy(szBareAddr, pszIPV6Addr, ulBytesToCopy);
        szBareAddr[ulBytesToCopy] = '\0';
        pszBareAddr = szBareAddr;
    }
    else
    {
        pszBareAddr = pszIPV6Addr;
    }

    if (hx_inet_pton(AF_INET6, (const char*)pszBareAddr, &m_addr.sin6_addr) <= 0)
    {
        res = HXR_FAIL;
    }

    return res;

}

STDMETHODIMP_(HXBOOL)
CHXSockAddrIN6::IsEqualAddr(IHXSockAddr* pOther)
{
    HXBOOL rc = FALSE;

    sockaddr_storage ss;
    sockaddr_in6* psa6;

    if (SUCCEEDED(GetNativeAddr(pOther, &ss, &psa6)))
    {
        rc = IN6_ARE_ADDR_EQUAL(&m_addr.sin6_addr, &psa6->sin6_addr);
    }

    return rc;
}

STDMETHODIMP_(UINT16)
CHXSockAddrIN6::GetPort(void)
{
    return hx_ntohs(m_addr.sin6_port);
}

STDMETHODIMP
CHXSockAddrIN6::SetPort(UINT16 port)
{
    HX_ASSERT(m_lCount == 1);
    m_addr.sin6_port = hx_htons(port);
    return HXR_OK;
}

STDMETHODIMP_(HXBOOL)
CHXSockAddrIN6::IsEqualPort(IHXSockAddr* pOther)
{
    if ((pOther->GetFamily() != HX_SOCK_FAMILY_IN4)
    &&  (pOther->GetFamily() != HX_SOCK_FAMILY_IN6))
    {
        return FALSE;
    }

    return (GetPort() == pOther->GetPort());
}

STDMETHODIMP
CHXSockAddrIN6::MaskAddr(UINT32 nBits)
{
    HX_ASSERT(m_lCount == 1);
    if (!hx_maskaddr6(&m_addr.sin6_addr, nBits))
    {
        return HXR_FAIL;
    }
    return HXR_OK;
}

STDMETHODIMP_(HXBOOL)
CHXSockAddrIN6::IsEqualNet(IHXSockAddr* pOther, UINT32 nBits)
{
    HXBOOL rc = FALSE;

    sockaddr_storage ss;
    sockaddr_in6* psa6;

    if (SUCCEEDED(GetNativeAddr(pOther, &ss, &psa6)))
    {
        struct in6_addr thisaddr, thataddr;
        memcpy(&thisaddr, &m_addr.sin6_addr, sizeof(struct in6_addr));
        memcpy(&thataddr, &psa6->sin6_addr, sizeof(struct in6_addr));
        if (hx_maskaddr6(&thisaddr, nBits) && hx_maskaddr6(&thataddr, nBits))
        {
            rc = IN6_ARE_ADDR_EQUAL(&thisaddr, &thataddr);
        }
    }

    return rc;
}

STDMETHODIMP
CHXSockAddrIN6::GetFullAddr(IHXBuffer** ppBuf)
{
    // We would like to use s6_addr16 but some systems don't have it.
    // Therefore, we must trust that s6_addr is (at least) 16-bit aligned.
    char szAddr[HX_ADDRSTRLEN_IN6];
    UINT16* p = (UINT16*)m_addr.sin6_addr.s6_addr;
    sprintf(szAddr, "%4hx:%4hx:%4hx:%4hx:%4hx:%4hx:%4hx:%4hx",
            hx_ntohs(p[0]), hx_ntohs(p[1]),
            hx_ntohs(p[2]), hx_ntohs(p[3]),
            hx_ntohs(p[4]), hx_ntohs(p[5]),
            hx_ntohs(p[6]), hx_ntohs(p[7]));
    CHXBuffer::FromCharArray(szAddr, ppBuf);
    return HXR_OK;
}

STDMETHODIMP
CHXSockAddrIN6::GetFullAddrZ(IHXBuffer** ppBuf)
{
    // We would like to use s6_addr16 but some systems don't have it.
    // Therefore, we must trust that s6_addr is (at least) 16-bit aligned.
    UINT16* p = (UINT16*)m_addr.sin6_addr.s6_addr;
    char szAddr[HX_ADDRSTRLEN_IN6];
    sprintf(szAddr, "%04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx",
            hx_ntohs(p[0]), hx_ntohs(p[1]),
            hx_ntohs(p[2]), hx_ntohs(p[3]),
            hx_ntohs(p[4]), hx_ntohs(p[5]),
            hx_ntohs(p[6]), hx_ntohs(p[7]));
    CHXBuffer::FromCharArray(szAddr, ppBuf);
    return HXR_OK;
}

STDMETHODIMP
CHXSockAddrIN6::ExtractIN4Addr(IHXSockAddrIN4** ppAddr)
{
    sockaddr_in sa4;

    if (!IN6_IS_ADDR_V4MAPPED(&m_addr.sin6_addr) &&
        !IN6_IS_ADDR_V4COMPAT(&m_addr.sin6_addr))
    {
        *ppAddr = NULL;
        return HXR_FAIL;
    }
    memset(&sa4, 0, sizeof(sa4));
    sa4.sin_family = AF_INET;
    memcpy(&sa4.sin_addr.s_addr, &m_addr.sin6_addr.s6_addr[12], 4);
    sa4.sin_port = m_addr.sin6_port;
    *ppAddr = new CHXSockAddrIN4(m_pContext, &sa4);
    if (*ppAddr == NULL)
    {
        return HXR_OUTOFMEMORY;
    }
    (*ppAddr)->AddRef();
    return HXR_OK;
}


STDMETHODIMP_(HXIN6AddrClass)
CHXSockAddrIN6::GetAddrClass(void)
{
    if (IN6_IS_ADDR_UNSPECIFIED(&m_addr.sin6_addr))
    {
        return HX_IN6_CLASS_ANY;
    }
    if (IN6_IS_ADDR_LOOPBACK(&m_addr.sin6_addr))
    {
        return HX_IN6_CLASS_LOOPBACK;
    }
    if (IN6_IS_ADDR_MULTICAST(&m_addr.sin6_addr))
    {
        return HX_IN6_CLASS_MULTICAST;
    }
    if (IN6_IS_ADDR_LINKLOCAL(&m_addr.sin6_addr))
    {
        return HX_IN6_CLASS_LINKLOCAL;
    }
    if (IN6_IS_ADDR_SITELOCAL(&m_addr.sin6_addr))
    {
        return HX_IN6_CLASS_SITELOCAL;
    }
    if ((m_addr.sin6_addr.s6_addr[0] & 0xe0) == 0x20)
    {
        return HX_IN6_CLASS_UNICAST;
    }
    if (IN6_IS_ADDR_V4MAPPED(&m_addr.sin6_addr))
    {
        return HX_IN6_CLASS_V4MAPPED;
    }
    if (IN6_IS_ADDR_V4COMPAT(&m_addr.sin6_addr))
    {
        return HX_IN6_CLASS_V4COMPAT;
    }
    return HX_IN6_CLASS_NONE;
}

STDMETHODIMP_(UINT32)
CHXSockAddrIN6::GetFlowInfo(void)
{
    return hx_ntohl(m_addr.sin6_flowinfo);
}

STDMETHODIMP
CHXSockAddrIN6::SetFlowInfo(UINT32 uFlowInfo)
{
    HX_ASSERT(m_lCount == 1);
    m_addr.sin6_flowinfo = hx_htonl(uFlowInfo);
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
CHXSockAddrIN6::GetScopeId(void)
{
#ifdef MISSING_SOCKADDR_IN6_SCOPE_ID
    return 0;
#else
    return hx_ntohl(m_addr.sin6_scope_id);
#endif
}

STDMETHODIMP
CHXSockAddrIN6::SetScopeId(UINT32 uScopeId)
{
    HX_ASSERT(m_lCount == 1);
#ifdef MISSING_SOCKADDR_IN6_SCOPE_ID
    return HXR_UNEXPECTED;
#else
    m_addr.sin6_scope_id = hx_htonl(uScopeId);
    return HXR_OK;
#endif
}

HX_RESULT
CHXSockAddrIN6::GetNativeAddr(IHXSockAddr* pAddr,
                              sockaddr_storage* pss,
                              sockaddr_in6** ppsa)
{
    HX_ASSERT(pss);

    size_t salen; // Dummy var-- not really used for anything.
    sockaddr* psa;

    IHXSockAddrNative* pNative = NULL;

    HX_RESULT hxr = pAddr->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);

    if (SUCCEEDED(hxr))
    {
        pNative->Get(&psa, &salen);

        if (psa->sa_family == AF_INET)
        {
            sockaddr_in6* psa6 = (sockaddr_in6*)pss;
            hx_map4to6(psa, psa6);

            *ppsa = (sockaddr_in6*)pss;
        }
        else if (psa->sa_family == AF_INET6)
        {
            *ppsa = (sockaddr_in6*)psa;
        }
        else
        {
            // Can't convert to IN6 addr
            hxr = HXR_SOCK_AFNOSUPPORT;
        }

        HX_RELEASE(pNative);
    }
    return hxr;
}

//private function.
//Assumes that pScopeId is valid ptr(Not Null) and points to scopeid.
HX_RESULT
CHXSockAddrIN6::SetScopeIDFromBuffer(const char* pScopeId)
{
    HX_ASSERT(pScopeId);

    HX_RESULT hxr = HXR_OK;
    UINT32 uiscopeid = 0;
    char* pEnd = NULL;
    if (*pScopeId)
    {
        uiscopeid = strtol(pScopeId, &pEnd, 10);

        if ((uiscopeid == LONG_MAX) || (uiscopeid == LONG_MIN))
        {
            //out of range.
            return HXR_FAIL;
        }

        if (pEnd && *pEnd)
        {
            //We failed to convert pScopeId to long.
            //Check if it is a interface name.
#ifdef _UNIX
            uiscopeid = if_nametoindex(pScopeId);
            if (uiscopeid == 0)
            {
                return HXR_FAIL;
            }
#else
            return HXR_FAIL;
#endif
        }

        hxr = SetScopeId( uiscopeid );
    }

    return hxr;
}
