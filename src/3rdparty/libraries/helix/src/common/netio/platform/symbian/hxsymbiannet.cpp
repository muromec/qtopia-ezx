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

#include "hxsymbiannet.h"
#include "hxsymbianresolver.h"
#include "hxsymbiansocket.h"
#include "hxsymbiansockaddr.h"

#include "hxassert.h"


struct HXSockFamilyTable
{
    HXSockFamily    hx;
    TUint       sym;
};

struct HXTypeTable
{
    HXSockType  hx;
    TUint       sym;
};

struct HXSockProtocolTable
{
    HXSockProtocol  hx;
    TUint       sym;
};

static const HXSockFamilyTable g_familyTable[] =
{
    {HX_SOCK_FAMILY_NONE,       KAFUnspec       }
    ,{HX_SOCK_FAMILY_LOCAL,     KAFUnspec       }
    ,{HX_SOCK_FAMILY_IN4,       KAfInet         }
    ,{HX_SOCK_FAMILY_IN6,       KAfInet6        } 
    ,{HX_SOCK_FAMILY_INANY,     KAfInet         }
    ,{HX_SOCK_FAMILY_LBOUND,    KAFUnspec       }
    ,{HX_SOCK_FAMILY_CLOAK,     KAFUnspec       }
    ,{HX_SOCK_FAMILY_LAST,      KAFUnspec       }
};

static const HXTypeTable g_typeTable[] =
{
    {HX_SOCK_TYPE_NONE,         0               }
    ,{HX_SOCK_TYPE_RAW,         KSockRaw        }
    ,{HX_SOCK_TYPE_UDP,         KSockDatagram   }
    ,{HX_SOCK_TYPE_TCP,         KSockStream     }
    ,{HX_SOCK_TYPE_MCAST,       KSockDatagram   }
    ,{HX_SOCK_TYPE_SCTP,        0               }
    ,{HX_SOCK_TYPE_DCCP,        0               }
    ,{HX_SOCK_TYPE_LAST,        0               }
        
};

static const HXSockProtocolTable g_protocolTable[] =
{
    {HX_SOCK_PROTO_NONE,    0                   }
    ,{HX_SOCK_PROTO_ANY,    0                   }
    ,{HX_SOCK_PROTO_LAST,   0                   }
};


CHXClientNetServices* CreateClientNetServices(IUnknown* pContext)
{
    CHXClientNetServices* pClientNetServices	=  new CHXClientNetServices();
    if(pClientNetServices)
    {
        pClientNetServices->Init(pContext);
    }
    return pClientNetServices;
}

TUint
CHXNetServices::GetNativeFamily(HXSockFamily hxfamily)
{
    TUint family = KAfInet;
    int index, tblLength = sizeof(g_familyTable)/sizeof(HXSockFamilyTable);
    for (index=0; index < tblLength; index++)
    {
        if(g_familyTable[index].hx == hxfamily)
        {
            family = g_familyTable[index].sym;
            break;
        }
    }
    return family;
}

TUint
CHXNetServices::GetNativeSockType(HXSockType hxtype)
{
    TUint type = KSockStream;
    int index, tblLength = sizeof(g_typeTable)/sizeof(HXTypeTable);
    for (index=0; index < tblLength; index++)
    {
        if(g_typeTable[index].hx == hxtype)
        {
            type = g_typeTable[index].sym;
            break;
        }
    }
    return type;
}

TUint
CHXNetServices::GetNativeProtocol(HXSockProtocol hxproto)
{
    TUint proto = KProtocolInetTcp;
    int index, tblLength = sizeof(g_protocolTable)/sizeof(HXSockProtocolTable);
    for (index=0; index < tblLength; ++index)
    {
        if(g_protocolTable[index].hx == hxproto)
        {
            proto = g_protocolTable[index].sym;
            break;
        }
    }
    return proto;
}

CHXNetServices::CHXNetServices(void) :
    m_nRefCount(0)
    ,m_pContext(NULL)
{
}

CHXNetServices::~CHXNetServices(void)
{
    Close();
}

STDMETHODIMP
CHXNetServices::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXNetServices*)this },
        { GET_IIDHANDLE(IID_IHXNetServices), (IHXNetServices*) this },
    };

    HX_RESULT res = QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);

    if ((HXR_OK != res) && m_pAccessPointMan)
    {
        res = m_pAccessPointMan->QueryInterface(riid, ppvObj);
    }

    return res;
}


STDMETHODIMP_(ULONG32)
CHXNetServices::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CHXNetServices::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

HX_RESULT
CHXNetServices::Init(IUnknown* pContext)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;
    if(pContext)
    {
        HX_RELEASE(m_pContext);
        m_pContext = pContext;
        m_pContext->AddRef();
        HXSymbianAccessPointManager *pAPManager;
        pAPManager = HXSymbianAccessPointManager::CreateObject();
        if (pAPManager)
        {
            IUnknown* pOuter = (IUnknown*)(IHXNetServices*)this;

            res = pAPManager->SetupAggregation(pOuter,
                          &m_pAccessPointMan);
            if (HXR_OK == res)
            {
                pAPManager->SetContext(pContext);
            }
            else
            {
                pAPManager->AddRef();
                HX_RELEASE(pAPManager);
            }
            res = HXR_OK;
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }
    return res; 
}

void
CHXNetServices::Close(void)
{
    HX_RELEASE(m_pAccessPointMan);
    HX_RELEASE(m_pContext);
}

STDMETHODIMP
CHXNetServices::CreateResolver(IHXResolve** ppResolver)
{
    HX_RESULT res = HXR_OK; 
    *ppResolver = new CHXResolver(this, m_pContext);
    if (*ppResolver)
    {
        (*ppResolver)->AddRef();
    }
    else
    {
        res = HXR_OUTOFMEMORY;
    }
    return res;
}

STDMETHODIMP
CHXNetServices::CreateSockAddr(HXSockFamily f, IHXSockAddr** ppAddr)
{
    IHXSockAddr* pAddr = NULL;
    switch (f)
    {
        case HX_SOCK_FAMILY_IN4:
            pAddr = new CHXSockAddrIN4(m_pContext);
            break;
        case HX_SOCK_FAMILY_IN6:
            pAddr = new CHXSockAddrIN6(m_pContext);
            break;
        case HX_SOCK_FAMILY_INANY:
            pAddr = new CHXSockAddrIN6(m_pContext);
            break;
        default:
            HX_ASSERT(FALSE);
            *ppAddr = NULL;
            return HXR_UNEXPECTED;
    }
    if (pAddr == NULL)
    {
        return HXR_OUTOFMEMORY;
    }

    pAddr->QueryInterface(IID_IHXSockAddr, (void**)ppAddr);
    return HXR_OK;
}

STDMETHODIMP
CHXNetServices::CreateListeningSocket(IHXListeningSocket** ppSock)
{
    HX_RESULT hxr = HXR_FAILED;
    IHXSocket* pActualSock = NULL;
    if (SUCCEEDED(CreateSocket(&pActualSock)))
    {
        *ppSock = new CHXListeningSocket(pActualSock);
        if (*ppSock)
        {
            (*ppSock)->AddRef();
            hxr = HXR_OK;
        }
        else
        {
            HX_RELEASE(pActualSock);
            hxr = HXR_OUTOFMEMORY;
        }
    }
    return hxr;
}


STDMETHODIMP
CHXNetServices::CreateSocket(IHXSocket** ppSock)
{
    HX_RESULT res = HXR_OK;
    *ppSock = new CHXSymbianSocket(this, m_pContext);
    if(*ppSock)
    {
        (*ppSock)->AddRef();
    }
    else
    {
        res = HXR_OUTOFMEMORY;
    }
    return res;
}

