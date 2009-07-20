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
#include "hxslist.h"
#include "hxbuffer.h"
#include "safestring.h"
#include "debug.h"
#include "hxassert.h"
#include "netbyte.h"
#include "hxheap.h"
#include "hxnet.h"
#include "hxinetaddr.h"
#include "hxsockutil.h"


#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif


#define D_SOCKUTIL D_INFO //XXXLCM



//
// For copying sock addr collection info passed back by IHXResolveResponse.
// Use FreeAddrVec() when you are done.
//
// XXXLCM something like IHXSockAddrList (GetCount, GetAt, etc.) would be
//        nicer/safer. Or a generic IHXList (of IUnknown* or void*)
//
// family is a filter that specifies which addresses to extract
//
//
HX_RESULT HXSockUtil::AllocAddrVec(IHXSockAddr** ppAddrVec,
                                      UINT32& nVecLen,
                                      IHXSockAddr**& ppAddrVecOut,
                                      HXSockFamily familyOut,
                                      bool convert,
                                      IHXNetServices* pNetServices)
{
    HX_ASSERT(nVecLen > 0);
    HX_ASSERT(ppAddrVec);
    HX_ASSERT(familyOut == HX_SOCK_FAMILY_INANY || familyOut == HX_SOCK_FAMILY_IN4 || familyOut == HX_SOCK_FAMILY_IN6);

    HX_RESULT hxr = HXR_OUTOFMEMORY;
    ppAddrVecOut = new IHXSockAddr*[nVecLen];
    if(ppAddrVecOut)
    {
        UINT32 idxOut = 0;
        for(UINT32 idxSource = 0; idxSource < nVecLen; ++idxSource)
        {
            IHXSockAddr* pOther = ppAddrVec[idxSource];
            if(familyOut == HX_SOCK_FAMILY_INANY || familyOut == pOther->GetFamily())
            {
                // no conversion needed (family matches) or family out is 'any'
                ppAddrVecOut[idxOut] = ppAddrVec[idxSource];
                HX_ASSERT(ppAddrVecOut[idxOut]);
                ppAddrVecOut[idxOut]->AddRef();
                ++idxOut;
            }
            else if (convert)
            {
                // convert to family
                HX_ASSERT(pNetServices);
                IHXSockAddr* pAddr = 0;
                HX_RESULT hrConvert = ConvertAddr(pNetServices, familyOut, pOther, pAddr);
                if(SUCCEEDED(hrConvert))
                {
                    ppAddrVecOut[idxOut] = pAddr;
                    HX_ASSERT(ppAddrVecOut[idxOut]);
                    ++idxOut;
                }
            }


        }
        nVecLen = idxOut;
        if( nVecLen > 0)
        {
            hxr = HXR_OK;
        }
        else
        {
            //
            // Most likely case is output family is IPv4 and source vector contains
            // only non-mapped IPv6 addresss that can't be converted.
            //
            HX_VECTOR_DELETE(ppAddrVecOut);
            hxr = HXR_FAIL;
        }


    }
    return hxr;
}
// see AllocAddrVec()
void HXSockUtil::FreeAddrVec(IHXSockAddr**& ppAddrVec, UINT32& nVecLen)
{
    if(ppAddrVec)
    {
        HX_ASSERT(nVecLen > 0);
        for(UINT32 idx = 0; idx < nVecLen; ++idx)
        {
            ppAddrVec[idx]->Release();
        }
        HX_VECTOR_DELETE(ppAddrVec);
    }
}

static /* private */
bool MatchClassIN6(IHXSockAddrIN6* pAddr6, HXIN4AddrClass addrClassIN4, HXIN6AddrClass addrClassIN6)
{
    HX_ASSERT(pAddr6);

    bool isMatch = false;

    HXIN6AddrClass addrClassThis = pAddr6->GetAddrClass();
    if (HX_IN6_CLASS_V4MAPPED == addrClassThis)
    {
        //check IN4 class for mapped IN4 addr
        IHXSockAddrIN4* pAddr4 = 0;
        pAddr6->ExtractIN4Addr(&pAddr4);
        HX_ASSERT(pAddr4);
        isMatch = (pAddr4->GetAddrClass() == addrClassIN4);
        HX_RELEASE(pAddr4);
    }
    else
    {
        isMatch = (addrClassThis == addrClassIN6);
    }

    return isMatch;
}

static /* private */
bool MatchClass(IHXSockAddr* pAddr, HXIN4AddrClass addrClassIN4, 
                HXIN6AddrClass addrClassIN6)
{
    HX_ASSERT(HX_IN6_CLASS_V4MAPPED != addrClassIN6);

    bool isMatch = false;
    HX_RESULT hxr = HXR_FAILED;
    
    IHXSockAddrIN6* pAddr6 = 0;
    hxr = pAddr->QueryInterface(IID_IHXSockAddrIN6, 
                    reinterpret_cast<void**>(&pAddr6));
    if(SUCCEEDED(hxr))
    {
        isMatch = MatchClassIN6(pAddr6, addrClassIN4, addrClassIN6);
        HX_RELEASE(pAddr6);
    }
    else
    {
        IHXSockAddrIN4* pAddr4 = 0;
        hxr = pAddr->QueryInterface(IID_IHXSockAddrIN4, 
                        reinterpret_cast<void**>(&pAddr4));
        if(SUCCEEDED(hxr))
        {
            isMatch = (addrClassIN4 == pAddr4->GetAddrClass());
            HX_RELEASE(pAddr4);
        }
    }
    return isMatch;
}

bool HXSockUtil::IsAnyAddrIN6(IHXSockAddrIN6* pAddr)
{
    return MatchClassIN6(pAddr, HX_IN4_CLASS_ANY, HX_IN6_CLASS_ANY);
}

bool HXSockUtil::IsMulticastAddrIN6(IHXSockAddrIN6* pAddr)
{
    return MatchClassIN6(pAddr, HX_IN4_CLASS_MULTICAST, HX_IN6_CLASS_MULTICAST);
}

bool HXSockUtil::IsAnyAddr(IHXSockAddr* pAddr)
{
    return MatchClass(pAddr, HX_IN4_CLASS_ANY, HX_IN6_CLASS_ANY);
}

bool HXSockUtil::IsMulticastAddr(IHXSockAddr* pAddr)
{
    return MatchClass(pAddr, HX_IN4_CLASS_MULTICAST, HX_IN6_CLASS_MULTICAST);
}


HX_RESULT HXSockUtil::CreateSocket(IHXNetServices* pNetSvc, IHXSocketResponse* pResp, HXSockFamily f,
                                          HXSockType t, HXSockProtocol p,
                                          IHXSocket*& pSock)
{
    HX_RESULT hxr = pNetSvc->CreateSocket(&pSock);
    if(HXR_OK == hxr)
    {
        pSock->SetResponse(pResp);
        hxr = pSock->Init(f,t,p);
        if(HXR_OK != hxr)
        {
            HX_RELEASE(pSock);
        }
    }
    return hxr;

}

HX_RESULT HXSockUtil::CreateAddr(IHXNetServices* pServices, HXSockFamily family,
                                                 const char* pszAddr, UINT16 port,
                                                 IHXSockAddr*& pAddrOut /*out*/)
{
    HX_ASSERT(pServices);
    HX_RESULT hxr = pServices->CreateSockAddr(family, &pAddrOut);
    if(SUCCEEDED(hxr))
    {
        if(pszAddr)
        {
            HX_ASSERT(*pszAddr);
            IHXBuffer* pBuf;
            CHXBuffer::FromCharArray(pszAddr, &pBuf);
            pAddrOut->SetAddr(pBuf);
            HX_RELEASE(pBuf);
        }
        pAddrOut->SetPort(port);
    }

    return hxr;

}

HX_RESULT HXSockUtil::SetAddr(IHXSockAddr* pAddr /*modified*/, 
                              const char* pAddrString, 
                              UINT16 port)
{
    HX_ASSERT(pAddr);
    HX_ASSERT(pAddrString);

    IHXBuffer* pBuf = 0;
    HX_RESULT hxr = CHXBuffer::FromCharArray(pAddrString, &pBuf);
    if (HXR_OK == hxr)
    {
        hxr = pAddr->SetAddr(pBuf);
        if (HXR_OK == hxr)
        {
            hxr = pAddr->SetPort(port);
        }
        HX_RELEASE(pBuf);
    }
    
    return hxr;
}

HX_RESULT HXSockUtil::SetAddrNetOrder(IHXSockAddr* pAddr /*modified*/,
                                short family,
                                const void* pAddrData /*net order octets/hextets*/,
                                UINT16 port /*net order*/)
{
    HX_ASSERT(pAddrData);
    char szAddr[HX_ADDRSTRLEN]; // big enough for ipv6 and ipv4
    hx_inet_ntop(family, pAddrData, szAddr, HX_ADDRSTRLEN);
    return SetAddr(pAddr, szAddr, WToHost(port));
}


HX_RESULT HXSockUtil::GetIN4Address(IHXSockAddr* pAddr, REF(UINT32) rulAddr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pAddr && pAddr->GetFamily() == HX_SOCK_FAMILY_IN4)
    {
        IHXBuffer* pBuf = NULL;
        retVal = pAddr->GetAddr(&pBuf);
        if (SUCCEEDED(retVal))
        {
            UINT32 ulVal[4] = {0, 0, 0, 0};
            INT32  lNumArg  = sscanf((const char*)pBuf->GetBuffer(),
                                     "%lu.%lu.%lu.%lu",
                                     &ulVal[0], &ulVal[1],
                                     &ulVal[2], &ulVal[3]);
            if (lNumArg == 4)
            {
                rulAddr = (ulVal[0] << 24) | (ulVal[1] << 16) | (ulVal[2] << 8) | ulVal[3];
            }
            else
            {
                retVal = HXR_FAIL;
            }
        }
        HX_RELEASE(pBuf);
    }

    return retVal;
}

HX_RESULT HXSockUtil::GetIN4Port(IHXSockAddr* pAddr, REF(UINT16) rusPort)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pAddr && pAddr->GetFamily() == HX_SOCK_FAMILY_IN4)
    {
        rusPort = pAddr->GetPort();
        retVal = HXR_OK;
    }

    return retVal;
}

// convert IPv4 to IPv6 V4MAPPED
HX_RESULT HXSockUtil::Convert4to6(IHXNetServices* pNetServices, IHXSockAddr* pAddrIn, IHXSockAddr*& pAddrOut)
{
    HX_ASSERT(pAddrIn);
    HX_RESULT hxr = HXR_FAIL;
    if (pAddrIn->GetFamily() == HX_SOCK_FAMILY_IN4)
    {
        IHXBuffer* pbuff = NULL;
        pAddrIn->GetAddr(&pbuff);
        HX_ASSERT(pbuff);
        hxr = CreateAddr(pNetServices, HX_SOCK_FAMILY_IN6, (const char*)pbuff->GetBuffer(), pAddrIn->GetPort(), pAddrOut); 
        HX_RELEASE(pbuff);
    }
    return hxr;
}

// convert assumed IPv6 V4MAPPED to IPv4 
HX_RESULT HXSockUtil::Convert6to4(IHXSockAddr* pAddrIn, IHXSockAddr*& pAddrOut)
{
    HX_ASSERT(pAddrIn);

    IHXSockAddrIN6* pAddr6 = NULL;
    HX_RESULT hxr = pAddrIn->QueryInterface(IID_IHXSockAddrIN6, (void**)&pAddr6);
    if (SUCCEEDED(hxr))
    {
        IHXSockAddrIN4* pAddr4 = NULL;
        hxr = pAddr6->ExtractIN4Addr(&pAddr4);
        if (SUCCEEDED(hxr))
        {
            hxr = pAddr4->QueryInterface(IID_IHXSockAddr, (void**)&pAddrOut);
            HX_RELEASE(pAddr4);
        }
        HX_RELEASE(pAddr6);
    }
    return hxr;
}


//
// for creating IPv6 addr (HX_IN6_CLASS_V4MAPPED) from IPv4 and vice-versa
//
HX_RESULT HXSockUtil::ConvertAddr(IHXNetServices* pNetServices,
                                    HXSockFamily familyOut,
                                    IHXSockAddr* pOld,
                                    IHXSockAddr*& pAddrOut /*out*/)
{
    HX_ASSERT(pNetServices);
    HX_ASSERT(pOld);

    HXSockFamily oldFamily = pOld->GetFamily();
    if(oldFamily != HX_SOCK_FAMILY_IN6 && oldFamily != HX_SOCK_FAMILY_IN4)
    {
        // we only convert from IPv6 or IPv4
        return HXR_FAIL;
    }

    HX_RESULT hxr = HXR_FAIL;

    if(oldFamily != familyOut)
    {
        switch(familyOut)
        {
            case HX_SOCK_FAMILY_IN4:
                HX_ASSERT(oldFamily == HX_SOCK_FAMILY_IN6);
                hxr = Convert6to4(pOld, pAddrOut);
                break;
            case HX_SOCK_FAMILY_IN6:
                HX_ASSERT(oldFamily == HX_SOCK_FAMILY_IN4);
                hxr = Convert4to6(pNetServices, pOld, pAddrOut);
                break;
            default:
                hxr = HXR_FAIL;
                HX_ASSERT(false); // not supported
                break;
        }

    }
    else
    {
        hxr = HXR_OK;
        pAddrOut = pOld;
        pAddrOut->AddRef();
    }

    return hxr;
}

HX_RESULT HXSockUtil::CreateAddrIN4(IHXNetServices* pNetServices,
                     UINT32 addr,
                     UINT16 port,
                     IHXSockAddr*& pAddrOut /*out*/)
{
    HX_ASSERT(pNetServices);

    pAddrOut = 0;
    HX_RESULT hxr = pNetServices->CreateSockAddr(HX_SOCK_FAMILY_IN4, &pAddrOut);
    if(SUCCEEDED(hxr))
    {
        UINT32 netAddr = DwToNet(addr);
        UINT16 netPort = WToNet(port);
        SetAddrNetOrder(pAddrOut, AF_INET, &netAddr, netPort);
    }

    return hxr;
}


