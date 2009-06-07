/* ***** BEGIN LICENSE BLOCK *****
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxnet.h"
#include "hxprefs.h"
#include "hxprefutil.h"
#include "hxthread.h"
#include "chxminiccf.h"
#include "chxresolver.h"
#if defined (THREADS_SUPPORTED)
#include "hxthreadedsocket.h"
#endif
#include "thrdutil.h"
#include "chxclientsocket.h"
#include "chxclientnetservices.h"
#include "hxtlogutil.h"
#include "debug.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


CHXClientNetServices* CreateClientNetServices(IUnknown* pContext)
{
    CHXClientNetServices* pRet   = NULL;
    IHXPreferences*       pPrefs = NULL;

    HX_ASSERT(pContext);
    if( pContext )
    {
#if defined (THREADS_SUPPORTED)
        HXBOOL bUseThreadedSockets = TRUE;
        pContext->QueryInterface(IID_IHXPreferences, (void**) &pPrefs);
        if( pPrefs )
        {
            ReadPrefBOOL( pPrefs, "UseThreadedSockets", bUseThreadedSockets );
            HX_RELEASE( pPrefs );
        }
        
        if( bUseThreadedSockets )
        {
            pRet = new CHXNetThreadClientNetServices();
        }
#endif
        if( !pRet)
        {
            pRet = new CHXClientNetServices();
        }
    }
    
    return pRet;
}

/************************************************************

     CHXClientNetServicesBase

*************************************************************/



CHXClientNetServicesBase::CHXClientNetServicesBase()
{
}

CHXClientNetServicesBase::~CHXClientNetServicesBase()
{
}


STDMETHODIMP 
CHXClientNetServicesBase::CreateSocket(IHXSocket** ppSock)
{
    return CreateSocketHelper(ppSock);
}

HX_RESULT 
CHXClientNetServicesBase::CreateSocket  (HXSockFamily f,
                                         HXSockType t,
                                         HXSockProtocol p,
                                         HX_SOCK s,
                                         IHXSocket** ppSock)
{
    HXSOCKET_CREATE_PARAMS params;
    params.f = f;
    params.t = t;
    params.p = p;
    params.sock = s;

    return CreateSocketHelper(ppSock, &params);
}

HX_RESULT 
CHXClientNetServicesBase::CreateClientSocket(IHXSocket*& pSock, 
                                             CHXNetServices* pNetServices,
                                             const HXSOCKET_CREATE_PARAMS* pParams)
{
    HX_ASSERT(pNetServices);
    HX_RESULT hr = HXR_FAIL;
    CHXClientSocket* pClientSock = 0;
    if( pParams )
    {
        pClientSock = new CHXClientSocket(pNetServices, m_punkContext, pParams->f, pParams->t, pParams->p, pParams->sock);
    }
    else
    {
        pClientSock = new CHXClientSocket(pNetServices, m_punkContext);
    }

    if (pClientSock)
    {
        pClientSock->AddRef();
        hr = pClientSock->QueryInterface(IID_IHXSocket, (void**)&pSock);
        HX_RELEASE(pClientSock);
    }
    else
    {
        hr = HXR_OUTOFMEMORY;
    }
    return hr;
}



/************************************************************

     CHXClientNetServices

*************************************************************/

CHXClientNetServices::CHXClientNetServices()
{
}

CHXClientNetServices::~CHXClientNetServices()
{
}

STDMETHODIMP
CHXClientNetServices::RegisterContext(IUnknown* pIContext)
{
    return CHXClientNetServicesBase::RegisterContext(pIContext);
}

//
// helper called by IHXNetServices::CreateSocket() method
//
// create regular client sock or threaded socket based on preference
//
HX_RESULT 
CHXClientNetServices::CreateSocketHelper(IHXSocket** ppSock, 
                                         const HXSOCKET_CREATE_PARAMS* pParams)
{
    return CreateClientSocket(*ppSock, this, pParams);
}

STDMETHODIMP
CHXClientNetServices::CreateResolver(IHXResolve** ppResolver)
{
    CHXResolver* pResolver = new CHXResolver(m_punkContext, this);
    if (pResolver == NULL)
    {
        *ppResolver = NULL;
        return HXR_OUTOFMEMORY;
    }
    pResolver->QueryInterface(IID_IHXResolve, (void**)ppResolver);
    return HXR_OK;
}

#if defined (THREADS_SUPPORTED)

/************************************************************

     CHXNetThreadClientNetServices

*************************************************************/

CHXNetThreadClientNetServices::CHXNetThreadClientNetServices()
                              : m_ulDriverThreadID(0)
                              , m_pDriver(0)
{
}

CHXNetThreadClientNetServices::~CHXNetThreadClientNetServices()
{
    HX_RELEASE(m_pDriver);
}

STDMETHODIMP
CHXNetThreadClientNetServices::RegisterContext(IUnknown* pIContext)
{
    HX_RESULT   hr = HXR_FAIL;
    IHXThread*  pDriverThread = NULL;

    hr = CHXClientNetServices::RegisterContext(pIContext);
    if (HXR_OK == hr)
    {
        hr = DoDriverInit();
        if (HXR_OK == hr && 
            (pDriverThread = m_pDriver->GetDriverThread()))
        {
            hr = pDriverThread->GetThreadId(m_ulDriverThreadID);
        }
    }
    HX_RELEASE(pDriverThread);

    return hr;

}

HX_RESULT 
CHXNetThreadClientNetServices::CreateSocketHelper(IHXSocket** ppSock, 
                                                  const HXSOCKET_CREATE_PARAMS* pParams)
{
    HX_RESULT   hr = HXR_OK;

    HX_ASSERT(ppSock);
    HX_ASSERT(m_ulDriverThreadID);

    if (HXGetCurrentThreadID() == m_ulDriverThreadID)
    {
        // this ensures that sockets created on the net thread
        // are regular client sockets, not threaded sockets 
        hr = CHXClientNetServices::CreateSocketHelper(ppSock, pParams);
    }
    else
    {
        // create client socket that provides actual implementation
        IHXSocket* pActual = 0;
        HX_RESULT hr = CreateClientSocket(pActual, this, 0);
        if (SUCCEEDED(hr))
        {
            // create wrapper
            hr = HXThreadedSocket::Create(m_punkContext, pActual, *ppSock);
            HX_RELEASE(pActual);
            if (SUCCEEDED(hr) && pParams)
            {
                // note: we must call Init rather than using full ctor
                // for HXThreadedSocket; guarantees that net thread does
                // proper init of selector
                hr = (*ppSock)->Init(pParams->f, pParams->t, pParams->p);
            }
        }
    }

    return hr;

}

STDMETHODIMP
CHXNetThreadClientNetServices::CreateResolver(IHXResolve** ppResolver)
{
    HX_RESULT   hr = HXR_OK;

    if (HXGetCurrentThreadID() == m_ulDriverThreadID)
    {
        HX_ASSERT(false); //XXXLCM impl
        hr = HXR_NOTIMPL;
    }
    else
    {
        hr = CHXClientNetServices::CreateResolver(ppResolver);
    }

    return hr;
}

//
// Create and run the network thread. 
//
// It is not absolutely necessary to do this here. We could wait until 
// the first threaded socket is created. In that case the thread would
// terminate once the last socket is destroyed. The driver determines
// the response thread based on the thread that creates it.
// 
HX_RESULT
CHXNetThreadClientNetServices::DoDriverInit()
{
    
    HX_RESULT hr = HXThreadTaskDriver::GetInstance(m_pDriver, m_punkContext);

#if defined(_WINDOWS)
    if (SUCCEEDED(hr))
    {
        // set network thread priority //XXXLCM need platfrom generic thread priority definition
        UINT32 threadPriority;
        UINT32 flag = 0;
        ReadPrefUINT32(m_punkContext, "ThreadedSockThreadPriority", flag); // 0, 1, 2
        switch (flag)
        {
        case 0:
            threadPriority = THREAD_PRIORITY_NORMAL;
            break;
        case 1:
        default:
            threadPriority = THREAD_PRIORITY_ABOVE_NORMAL;
            break;
        case 2:
            threadPriority = THREAD_PRIORITY_HIGHEST;
            break;
        }
        HXLOGL3(HXLOG_NETW, "CHXClientNetServicesBase::Init(): PREF: net thread priority (ThreadedSockThreadPriority) = %lu", threadPriority);
        hr = m_pDriver->SetThreadPriority(threadPriority);
        HX_ASSERT(HXR_OK == hr);
    }
#endif

    return hr;
}

#endif // THREADS_SUPPORTED
