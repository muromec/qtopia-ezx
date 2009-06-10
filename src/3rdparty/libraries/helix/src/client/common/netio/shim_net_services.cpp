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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxnet.h"
#include "hxprefs.h"
#include "hxprefutil.h"
#include "hxengin.h"
#include "sockaddrimp.h"
#include "hxnetapi.h"
#include "threngin.h"
#include "shim_resolve.h"
#include "shim_listening_socket.h"
#include "shim_socket.h"
#include "shim_net_services.h"
#include "hxassert.h"
#define HELIX_FEATURE_LOGLEVEL_NONE
#include "hxtlogutil.h"

#ifdef _UNIX
#include "unix_net.h"
#endif

#if defined(_WINDOWS) || defined(_UNIX)
// Platform has new net services implementation (eventually all 
// platforms will support the new api and this will go away)
#define HELIX_CLIENT_IPV6_NET_API
#endif

#if defined(HELIX_CLIENT_IPV6_NET_API)
#include "chxclientnetservices.h"
#endif


// com interface implementation
BEGIN_INTERFACE_LIST(CHXClientNetServicesShim)
    INTERFACE_LIST_ENTRY(IID_IHXNetServices, IHXNetServices)
#ifdef HELIX_FEATURE_SECURE_SOCKET
    INTERFACE_LIST_ENTRY(IID_IHXSecureNetServices, IHXSecureNetServices)
#endif 
    INTERFACE_LIST_ENTRY(IID_IHXContextUser, IHXContextUser)
    INTERFACE_LIST_ENTRY_DELEGATE_BLIND( _InternalQI )
END_INTERFACE_LIST

CHXClientNetServicesShim::CHXClientNetServicesShim()
    : m_punkContext(NULL)
      , m_bUseShim(FALSE)
      , m_pOldNetServicesImp(NULL)
      , m_pOldNetServices(NULL)
      , m_pDefNetServices(NULL)
#if defined(_UNIX)        
#if defined(_UNIX_THREADED_NETWORK_IO)
      , m_bNetworkThreading(TRUE)
#endif
      , m_pScheduler(NULL)
      , m_pUnixNetworkPump(NULL)
#endif    
{
}

CHXClientNetServicesShim::~CHXClientNetServicesShim()
{
   Close();
}


#if defined(_UNIX)
void UnixNetworkPump(void* pARG)
{
    CHXClientNetServicesShim* it = (CHXClientNetServicesShim*)pARG;
    //Drive the networking in case we don't have a network thread or
    //it is turned off.
#if defined(_UNIX_THREADED_NETWORK_IO)
    if( !it->m_bNetworkThreading )
    {
        unix_TCP::process_idle();
    }
#else
    unix_TCP::process_idle();
#endif //_UNIX_THREADED_NETWORK_IO    
    it->m_pUnixNetworkPump->ScheduleRelative(it->m_pScheduler, 20);
}
#endif

STDMETHODIMP
CHXClientNetServicesShim::RegisterContext(IUnknown* pIContext)
{
    HX_RESULT hr = HXR_OK;

    HX_ASSERT(pIContext);
    if (!pIContext)
    {
        HX_ASSERT(FALSE);
        return HXR_INVALID_PARAMETER;
    }

    HX_ENABLE_LOGGING(pIContext);

    m_punkContext = pIContext;
    m_punkContext->AddRef();

#if defined(HELIX_CLIENT_IPV6_NET_API)
    CHXClientNetServices* pSvc = CreateClientNetServices(m_punkContext);
    if (pSvc)
    {
        pSvc->AddRef();
        hr  = pSvc->RegisterContext(m_punkContext);
        if (SUCCEEDED(hr))
        {
            hr = pSvc->QueryInterface(IID_IHXNetServices, (void**)&m_pDefNetServices);
        }
        HX_RELEASE(pSvc);

    }
    else
    {
        hr =  HXR_OUTOFMEMORY;
    }
    
    if (HXR_OK != hr)
    {
	goto exit;
    }

    // Get the IHXPreferences interface
    ReadPrefBOOL(m_punkContext, "UseNetServicesShim", m_bUseShim);

#else
    // XXX for now force this on for non-ipv6 platforms
    m_bUseShim = TRUE;

#if defined(_UNIX)        
    // Get the IHXPreferences interface
#if defined(_UNIX_THREADED_NETWORK_IO)
        ReadPrefBOOL( m_punkContext, "NetworkThreading", m_bNetworkThreading);
#endif        
        m_punkContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
        HX_ASSERT(m_pScheduler);
        m_pUnixNetworkPump = new CHXGenericCallback((void*)this, UnixNetworkPump);
        HX_ADDREF(m_pUnixNetworkPump);
        if( m_pUnixNetworkPump )
        {
            m_pUnixNetworkPump->ScheduleRelative(m_pScheduler, 20);
        }
        
#endif //_UNIX

#endif /* HELIX_CLIENT_IPV6_NET_API */

#if defined(HELIX_FEATURE_NET_LEGACYAPI) || defined(HELIX_FEATURE_NETSERVICES_SHIM)
    // Create old network services implementation
    if( m_bUseShim )
    {
        m_pOldNetServicesImp  = new HXNetworkServices(m_punkContext);
        if (NULL == m_pOldNetServicesImp)
        {
            hr = HXR_OUTOFMEMORY;
            goto exit;
        }

        HX_ASSERT(m_pOldNetServicesImp);
        HX_ADDREF(m_pOldNetServicesImp);

        hr = m_pOldNetServicesImp->QueryInterface(IID_IHXNetworkServices, (void**)&m_pOldNetServices);
        HX_ASSERT(m_pOldNetServices);

#if defined(THREADS_SUPPORTED)
        if (HXR_OK == hr)
        {
            // Start network thread for old implementation
            ThreadEngine::GetThreadEngine(m_punkContext);
        }
#endif /* THREADS_SUPPORTED */
    }
    
#endif /* HELIX_FEATURE_NET_LEGACYAPI */

exit:

    return hr;
}

HX_RESULT
CHXClientNetServicesShim::_InternalQI(REFIID riid, void** ppvObj)
{
    HX_RESULT	rc = HXR_NOINTERFACE;

    *ppvObj = NULL;

    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXNetServices*)this },
        { GET_IIDHANDLE(IID_IHXNetServices), (IHXNetServices*)this },
	{ GET_IIDHANDLE(IID_IHXContextUser), (IHXContextUser*)this }
    };

    rc = ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
    if (HXR_OK == rc)
    {
        return rc;
    }

    if (m_pOldNetServices && 
	HXR_OK == m_pOldNetServices->QueryInterface(riid, ppvObj))
    {
	return HXR_OK;
    }

    return rc;
}

void CHXClientNetServicesShim::Close(void)
{
#if defined(HELIX_CLIENT_IPV6_NET_API)
    CHXClientNetServices* pSvc = (CHXClientNetServices*)m_pDefNetServices;
    if (pSvc)
    {
        pSvc->Close();
    }
#endif

#if defined(HELIX_FEATURE_NET_LEGACYAPI) || defined(HELIX_FEATURE_NETSERVICES_SHIM) 
    if (m_pOldNetServicesImp)
    {
	m_pOldNetServicesImp->Close();
    }
#endif /* HELIX_FEATURE_NET_LEGACYAPI */

#ifdef _UNIX
    if( m_pUnixNetworkPump )
    {
        m_pUnixNetworkPump->Cancel(m_pScheduler);
    }
    HX_RELEASE(m_pUnixNetworkPump);
    HX_RELEASE(m_pScheduler);
#endif    

    HX_RELEASE(m_pDefNetServices);
    HX_RELEASE(m_pOldNetServices);
    HX_RELEASE(m_pOldNetServicesImp);
    HX_RELEASE(m_punkContext);

}

STDMETHODIMP CHXClientNetServicesShim::CreateResolver(IHXResolve** ppResolver)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_bUseShim)
    {
        if (ppResolver)
        {
            CHXResolveShim* pShim = new CHXResolveShim(m_punkContext, this, m_pOldNetServices);
            if (pShim)
            {
                retVal = pShim->QueryInterface(IID_IHXResolve, (void**) ppResolver);
            }
            if (FAILED(retVal))
            {
                HX_DELETE(pShim);
            }
        }
    }
    else if (m_pDefNetServices)
    {
        retVal = m_pDefNetServices->CreateResolver(ppResolver);
    }

    return retVal;
}

STDMETHODIMP CHXClientNetServicesShim::CreateSockAddr(HXSockFamily f, IHXSockAddr** ppAddr)
{
    if (m_bUseShim && (f == HX_SOCK_FAMILY_INANY) )
    {
        // shim only supports IPv4 (even if host machine support IPv6)
        f = HX_SOCK_FAMILY_IN4;
    }

    if (m_pDefNetServices)
    {
        return m_pDefNetServices->CreateSockAddr(f, ppAddr);
    }

    CHXSockAddrIN4* pAddr = NULL;
    pAddr = new CHXSockAddrIN4(m_punkContext);

    if (pAddr == NULL)
    {
        return HXR_OUTOFMEMORY;
    }
    pAddr->QueryInterface(IID_IHXSockAddr, (void**)ppAddr);
    return HXR_OK;
}

STDMETHODIMP CHXClientNetServicesShim::CreateListeningSocket(IHXListeningSocket** ppSock)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_bUseShim)
    {
        if (ppSock)
        {
            CHXListeningSocketShim* pShim = new CHXListeningSocketShim(m_punkContext, this, m_pOldNetServices);
            if (pShim)
            {
                retVal = pShim->QueryInterface(IID_IHXListeningSocket, (void**) ppSock);
            }
            if (FAILED(retVal))
            {
                HX_DELETE(pShim);
            }
        }
    }
    else if (m_pDefNetServices)
    {
        retVal = m_pDefNetServices->CreateListeningSocket(ppSock);
    }

    return retVal;
}

STDMETHODIMP CHXClientNetServicesShim::CreateSocket(IHXSocket** ppSock)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_bUseShim)
    {
        if (ppSock)
        {
            CHXClientSocketShim* pSocket = new CHXClientSocketShim(m_punkContext, this, m_pOldNetServices);
            if (pSocket)
            {
                retVal = pSocket->QueryInterface(IID_IHXSocket, (void**) ppSock);
            }
            if (FAILED(retVal))
            {
                HX_DELETE(pSocket);
            }
        }
    }
    else if (m_pDefNetServices)
    {
        retVal = m_pDefNetServices->CreateSocket(ppSock);
    }

    return retVal;
}

#ifdef HELIX_FEATURE_SECURE_SOCKET
STDMETHODIMP CHXClientNetServicesShim::CreateSecureSocket(IHXSecureSocket** ppSock)
{
    HX_RESULT retVal = HXR_FAIL;
    if (m_pDefNetServices)
    {
        IHXSecureNetServices* pSecNetService = NULL;
        m_pDefNetServices->QueryInterface(IID_IHXSecureNetServices, (void**)&pSecNetService);
        if(pSecNetService)
        {
            retVal = pSecNetService->CreateSecureSocket(ppSock);
            HX_RELEASE(pSecNetService);
        }
    }
    return retVal;
}
#endif

