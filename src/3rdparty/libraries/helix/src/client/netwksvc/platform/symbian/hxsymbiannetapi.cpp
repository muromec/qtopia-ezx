/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsymbiannetapi.cpp,v 1.10 2007/07/06 21:58:23 jfinnecy Exp $
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

#include "platform/symbian/hxsymbiannetapi.h"
#include "platform/symbian/hxsymbianresolv.h"
#include "platform/symbian/hxsymbiantcpsock.h"
#include "platform/symbian/hxsymbianudpsock.h"
#include "platform/symbian/hxsymbianapman.h"
#include "hxcloakedsocket.h"

#include "hxccf.h"
#include "debug.h"

#define D_NETAPI 0x10000000

HXNetworkServices::HXNetworkServices(IUnknown* pContext) :
    m_lRefCount(0),
    m_pContext(0)
{
    if (pContext)
    {
	m_pContext = pContext;
	m_pContext->AddRef();
    }

    HXSymbianAccessPointManager* pAPManager =
	HXSymbianAccessPointManager::CreateObject();

    if (pAPManager)
    {
	IUnknown* pOuter = (IUnknown*)(IHXNetworkServices*)this;

	HX_RESULT res = pAPManager->SetupAggregation(pOuter,
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
    }
}

HXNetworkServices::~HXNetworkServices()
{
    HX_RELEASE(m_pAccessPointMan);
    HX_RELEASE(m_pContext);
}

    /*
     *  IUnknown methods
     */
STDMETHODIMP HXNetworkServices::QueryInterface(THIS_
					       REFIID riid,
					       void** ppvObj)
{
    DPRINTF(D_NETAPI, ("HXNetworkServices::QueryInterface()\n"));

    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXNetworkServices*)this },
	{ GET_IIDHANDLE(IID_IHXNetworkServices), (IHXNetworkServices*) this },
	{ GET_IIDHANDLE(IID_IHXCloakedNetworkServices), (IHXCloakedNetworkServices*) this },
    };

    HX_RESULT res = QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);

    if ((HXR_OK != res) && m_pAccessPointMan)
    {
	res = m_pAccessPointMan->QueryInterface(riid, ppvObj);
    }

    return res;
}

STDMETHODIMP_(ULONG32) HXNetworkServices::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)HXNetworkServices::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

    /*
     *	IHXNetworkServices methods
     */

    /************************************************************************
     *	Method:
     *	    IHXNetworkServices::CreateTCPSocket
     *	Purpose:
     *	    Create a new TCP socket.
     */
STDMETHODIMP HXNetworkServices::CreateTCPSocket(THIS_
				IHXTCPSocket**    /*OUT*/  ppTCPSocket)
{
    DPRINTF(D_NETAPI, ("HXNetworkServices::CreateTCPSocket()\n"));

    HX_RESULT res = HXR_FAILED;
    IHXCommonClassFactory* pCCF = 0;

    if (m_pContext &&
	(m_pContext->QueryInterface(IID_IHXCommonClassFactory,
				    (void**)&pCCF) == HXR_OK) && pCCF)
    {
	IHXResolver* pResolver = 0;
	res = CreateResolver(&pResolver);

	if (res == HXR_OK)
	{
	    *ppTCPSocket = new HXSymbianTCPSocket(pCCF, pResolver);

	    if (*ppTCPSocket)
	    {
		(*ppTCPSocket)->AddRef();
		res = HXR_OK;
	    }
	    else
	    {
		res = HXR_OUTOFMEMORY;
	    }
	}

	HX_RELEASE(pResolver);
    }

    HX_RELEASE(pCCF);

    return res;
}

    /************************************************************************
     *	Method:
     *	    IHXNetworkServices::CreateUDPSocket
     *	Purpose:
     *	    Create a new UDP socket.
     */
STDMETHODIMP HXNetworkServices::CreateUDPSocket(THIS_
				IHXUDPSocket**    /*OUT*/  ppUDPSocket)
{
    DPRINTF(D_NETAPI, ("HXNetworkServices::CreateUDPSocket()\n"));

    HX_RESULT res = HXR_FAILED;
    IHXCommonClassFactory* pCCF = 0;

    if (m_pContext &&
	(m_pContext->QueryInterface(IID_IHXCommonClassFactory,
				    (void**)&pCCF) == HXR_OK) &&
	pCCF)
    {
	*ppUDPSocket = new HXSymbianUDPSocket(pCCF);

	if (*ppUDPSocket)
	{
	    (*ppUDPSocket)->AddRef();
	    res = HXR_OK;
	}
	else
	{
	    res = HXR_OUTOFMEMORY;
	}
    }

    HX_RELEASE(pCCF);

    return res;
}

    /************************************************************************
     *	Method:
     *	    IHXNetworkServices::CreateListenSocket
     *	Purpose:
     *	    Create a new TCP socket that will listen for connections on a
     *	    particular port.
     */
STDMETHODIMP HXNetworkServices::CreateListenSocket(THIS_
				    IHXListenSocket** /*OUT*/ ppListenSocket)
{
    DPRINTF(D_NETAPI, ("HXNetworkServices::CreateListenSocket()\n"));

    return HXR_NOTIMPL;
}

    /************************************************************************
     *	Method:
     *	    IHXNetworkServices::CreateResolver
     *	Purpose:
     *	    Create a new resolver that can lookup host names
     */
STDMETHODIMP HXNetworkServices::CreateResolver(THIS_
			    	IHXResolver**    /*OUT*/     ppResolver)
{
    DPRINTF(D_NETAPI, ("HXNetworkServices::CreateResolver()\n"));

    HX_RESULT res = HXR_OUTOFMEMORY;

    *ppResolver = new HXSymbianResolver(m_pContext);

    if (*ppResolver)
    {
	(*ppResolver)->AddRef();
	res = HXR_OK;
    }

    return res;
}


    /*
     *	IHXNetworkCloakedServices methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCloakedNetworkServices::CreateClientCloakedSocket
     *	Purpose:
     *	    Create a new HTTP cloaked TCP socket for the client.
     */
STDMETHODIMP HXNetworkServices::CreateClientCloakedSocket(THIS_
			IHXTCPSocket**    /*OUT*/     ppTCPSocket)
{
    DPRINTF(D_NETAPI, ("HXNetworkServices::CreateClientCloakedSocket()\n"));
#if defined(HELIX_FEATURE_HTTPCLOAK)
    HX_RESULT res = HXR_FAILED;

    if (m_pContext)
    {
	*ppTCPSocket = (IHXTCPSocket*)new HXCloakedSocket(m_pContext,FALSE);

	if (*ppTCPSocket)
	{
	    (*ppTCPSocket)->AddRef();
	    res = HXR_OK;
	}
	else
	{
	    res = HXR_OUTOFMEMORY;
	}
    }

    return res;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_HTTPCLOAK */
}

    /************************************************************************
     *	Method:
     *	    IHXCloakedNetworkServices::CreateServerCloakedSocket
     *	Purpose:
     *	    Create a new HTTP cloaked TCP socket that will listen for
     *      connections on a particular port.
     */
STDMETHODIMP HXNetworkServices::CreateServerCloakedSocket(THIS_
			IHXListenSocket**    /*OUT*/     ppListenSocket)
{
    DPRINTF(D_NETAPI, ("HXNetworkServices::CreateServerCloakedSocket()\n"));

    return HXR_NOTIMPL;
}

void HXNetworkServices::Close()
{
    DPRINTF(D_NETAPI, ("HXNetworkServices::Close()\n"));

    HX_RELEASE(m_pContext);
}
