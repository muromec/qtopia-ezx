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
#include "hlxclib/stdlib.h"
#include "hxengin.h"
#include "netbyte.h"
#include "hxsockutil.h"
#include "hxtick.h"
#include "shim_utils.h"
#include "shim_resolve.h"
#include "hxassert.h"

CHXResolveShim::CHXResolveShim(IUnknown* pContext, 
                               IHXNetServices* pServices,
                               IHXNetworkServices* pOldServices)
{
    HX_ASSERT(pContext);
    HX_ASSERT(pServices);
    HX_ASSERT(pOldServices);

    m_lRefCount     = 0;
    m_bBigEndian    = TestBigEndian();
    m_pContext      = pContext;
    m_pServices     = pServices;
    m_pOldServices  = pOldServices;
    m_pResponse     = NULL;
    m_pResolver     = NULL;
    m_usServicePort = 0;

    m_pContext->AddRef();
    m_pServices->AddRef();
    m_pOldServices->AddRef();
}

CHXResolveShim::~CHXResolveShim()
{
    Close();
}

STDMETHODIMP CHXResolveShim::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT retVal = HXR_NOINTERFACE;

    if (ppvObj)
    {
        // Set default
        *ppvObj = NULL;
        // Switch on riid
        if (IsEqualIID(riid, IID_IUnknown))
        {
            AddRef();
            *ppvObj = (IUnknown*) (IHXResolverResponse*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXResolve))
        {
            AddRef();
            *ppvObj = (IHXResolve*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXResolverResponse))
        {
            AddRef();
            *ppvObj = (IHXResolverResponse*) this;
            retVal  = HXR_OK;
        }
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CHXResolveShim::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXResolveShim::Release()
{
    INT32 rc = InterlockedDecrement(&m_lRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return (ULONG32) rc;
}

STDMETHODIMP CHXResolveShim::Init(IHXResolveResponse* pResponse)
{
    HX_RESULT retVal = HXR_FAIL;
  
    if (pResponse)
    {
        HX_ASSERT(m_pOldServices);

        // Save the IHXResolveResponse interface
        HX_RELEASE(m_pResponse);
        m_pResponse = pResponse;
        m_pResponse->AddRef();

        // Create an IHXResolver
        HX_RELEASE(m_pResolver);
        retVal = m_pOldServices->CreateResolver(&m_pResolver);
        if (SUCCEEDED(retVal))
        {
            // Get our IHXResolverResponse interface
            IHXResolverResponse* pResolverResponse = NULL;
            retVal = QueryInterface(IID_IHXResolverResponse, (void**) &pResolverResponse);
            if (SUCCEEDED(retVal))
            {
                // Init the IHXResolver
                retVal = m_pResolver->Init(pResolverResponse);
            }
            HX_RELEASE(pResolverResponse);
        }
    

    }

    return retVal;
}

STDMETHODIMP CHXResolveShim::Close()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pServices);
    HX_RELEASE(m_pOldServices);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pResolver);
    return HXR_OK;
}

STDMETHODIMP CHXResolveShim::GetAddrInfo(const char* pNode, const char* pServ, IHXAddrInfo* pHints)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pResolver && pNode)
    {
        if (pServ)
        {
            // Convert the service port to an UINT16 (native order)
            m_usServicePort = (UINT16) atoi(pServ);
        }
        else
        {
            m_usServicePort = 0;
        }
       
        // Call IHXResolver::GetHostByName(). This will
        // return in GetHostByNameDone().
        retVal = m_pResolver->GetHostByName(pNode);
    }

    return retVal;
}

STDMETHODIMP CHXResolveShim::GetNameInfo(IHXSockAddr* pAddr, UINT32 uFlags)
{
    HX_RESULT retVal = HXR_NOTIMPL;

    return retVal;
}

STDMETHODIMP CHXResolveShim::GetHostByNameDone(HX_RESULT status, ULONG32 ulAddr)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pResponse && m_pServices)
    {
        // Did we succeed in getting the address?
        if (SUCCEEDED(status))
        {
            // IHXResolverResponse returns address in native order.
            
            // Create an IHXSockAddr with this address and port
            IHXSockAddr* pAddr = NULL;
            retVal = HXSockUtil::CreateAddrIN4(m_pServices, ulAddr, m_usServicePort, pAddr);
            if (SUCCEEDED(retVal))
            {
                m_pResponse->GetAddrInfoDone(HXR_OK, 1, &pAddr);
            }
            HX_RELEASE(pAddr);
        }
        else
        {
            // DNS lookup failure
            m_pResponse->GetAddrInfoDone(status, NULL, 0);
            // Clear the return value
            retVal = HXR_OK;
        }
    }

    return retVal;
}

