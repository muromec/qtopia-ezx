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
#include "nettypes.h"
#include "hxnet.h"
#include "hxslist.h"
#include "safestring.h"
#include "sockimp.h"
#include "sockaddrimp.h"
#include "netbyte.h"
#include "hxsockutil.h"
#include "hxposixsockutil.h"

#include "hxtlogutil.h"
#include "debug.h"
#include "hxassert.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

HX_RESULT HXPosixSockUtil::CreateAddrIN6(IHXNetServices* pNetServices,
                     const struct sockaddr_in6* psa,
                     IHXSockAddr*& pAddrOut /*out*/)
{

    HX_ASSERT(pNetServices);
    HX_ASSERT(psa);

    HX_RESULT hr = pNetServices->CreateSockAddr(HX_SOCK_FAMILY_IN6, &pAddrOut);
    if (SUCCEEDED(hr))
    {
        HXSockUtil::SetAddrNetOrder(pAddrOut, AF_INET6, &psa->sin6_addr, psa->sin6_port);
    }

    return hr;

}

HX_RESULT HXPosixSockUtil::GetSockAddr(IHXSockAddr* pAddr, sockaddr_storage& sa)
{
    HX_ASSERT(pAddr);
    IHXSockAddrNative* pNative = NULL;
    sockaddr* psa;
    size_t salen;
    HX_RESULT hr = pAddr->QueryInterface(IID_IHXSockAddrNative, (void**)&pNative);
    if (SUCCEEDED(hr))
    {    
        pNative->Get(&psa, &salen);
        memcpy(&sa, psa, salen);
        pNative->Release();
    }
    return hr;
}


HX_RESULT HXPosixSockUtil::CreateAddrIN4(IHXNetServices* pNetServices,
                     const struct sockaddr_in* psa,
                     IHXSockAddr*& pAddrOut /*out*/)
{
    HX_ASSERT(pNetServices);
    HX_ASSERT(psa);
  
    HX_RESULT hr = pNetServices->CreateSockAddr(HX_SOCK_FAMILY_IN4, &pAddrOut);
    if(SUCCEEDED(hr))
    {
        hr = HXSockUtil::SetAddrNetOrder(pAddrOut, AF_INET, &psa->sin_addr.s_addr, psa->sin_port);
    }

    return hr;
}



HX_RESULT HXPosixSockUtil::CreateAddr(IHXNetServices* pNetServices,
                     const struct sockaddr* pAddr,
                     IHXSockAddr*& pAddrOut /*out*/)
{
    HX_ASSERT(pNetServices);
    HX_ASSERT(pAddr);

    HX_RESULT hr = HXR_FAIL;
    switch (pAddr->sa_family)
    {
    case AF_INET:
        {
            const struct sockaddr_in* psa = reinterpret_cast<const struct sockaddr_in*>(pAddr);
            hr = CreateAddrIN4(pNetServices, psa, pAddrOut);
        }
        break;
    case AF_INET6:
        {
            const struct sockaddr_in6* psa = reinterpret_cast<const struct sockaddr_in6*>(pAddr);
            hr = CreateAddrIN6(pNetServices, psa, pAddrOut);
        }
        break;
    default:
        HX_ASSERT(false); //XXXLCM
        HXLOGL3(HXLOG_NETW, "CreateAddr(): unknown family = %u", pAddr->sa_family);
    }

    return hr;
}


//
// build list of IHSockAddr* from struct addrinfo
//
HX_RESULT HXPosixSockUtil::CreateAddrInfo(IHXNetServices* pNetServices,
                         const struct hostent* phe,
                         UINT16 port,
                         CHXSimpleList& list /*modified*/)
{
    HX_RESULT hr = HXR_FAIL;

    HX_ASSERT(pNetServices);
    HX_ASSERT(phe);
    HX_ASSERT(list.IsEmpty());


    for (UINT32 idx = 0; /* nothing*/; ++idx)
    {
        IHXSockAddr* pAddr = 0;

        const char* pAddrOctets = phe->h_addr_list[idx];
        if (!pAddrOctets)
        {
            // end of list
            break;
        }

        HX_ASSERT(phe->h_length == sizeof(UINT32));

        UINT32 addr = *(const UINT32*)(pAddrOctets);
        hr = HXSockUtil::CreateAddrIN4(pNetServices,  DwToHost(addr), port, pAddr);
        if (HXR_OK == hr)
        {
            HX_ASSERT(pAddr != 0);
            list.AddTail(pAddr);
        }
        else
        {
            // error - give up
            break;
        }
    }

    return hr;
}

//
// build list of IHSockAddr* from 'addrinfo'
//
HX_RESULT HXPosixSockUtil::CreateAddrInfo(IHXNetServices* pNetServices,
                         const struct addrinfo* pai,
                         CHXSimpleList& list /*modified*/)
{
    HX_RESULT hr = HXR_FAIL;

    HX_ASSERT(pNetServices);
    HX_ASSERT(pai);
    HX_ASSERT(list.IsEmpty());

    while (pai)
    {
        IHXSockAddr* pAddr = 0;
        hr = CreateAddr(pNetServices, pai->ai_addr, pAddr);

        if (HXR_OK == hr)
        {
            HX_ASSERT(pAddr != 0);
            list.AddTail(pAddr);
        }
        else
        {
            break;
        }

        pai = pai->ai_next;
    }
    return hr;
}














