/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsymbianresolv.cpp,v 1.11 2007/07/06 21:58:23 jfinnecy Exp $
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

#include "platform/symbian/hxsymbianresolv.h"
#include "debug.h"
#include "smartptr.h"
#include "hlxclib/string.h"
#include <in_sock.h>

#define D_RESOLVER 0x10000000

const ULONG32 DefaultRetryCount = 5;

HXSymbianResolver::HXSymbianResolver(IUnknown* pContext) :
    CActive(EPriorityStandard),
    m_lRefCount(0),
    m_pResponse(0),
    m_pAPManager(0),
    m_pAPResponse(0),
    m_bInitialized(FALSE),
    m_pHostname(0),
    m_ulRetryCount(0)
{
    CActiveScheduler::Add(this);

    if (pContext)
    {
	pContext->QueryInterface(IID_IHXAccessPointManager,
				 (void**)&m_pAPManager);
	
	if (m_pAPManager)
	{
	    m_pAPResponse = new HXAccessPointConnectResp(this, 
							 static_APConnectDone);
	    HX_ADDREF(m_pAPResponse);
	}
    }

    if ((m_sockServ.Connect() == KErrNone) &&
	(m_resolver.Open(m_sockServ, KAfInet, KProtocolInetTcp) == KErrNone))
    {
	m_bInitialized = TRUE;
    }
}

HXSymbianResolver::~HXSymbianResolver()
{
    if (m_bInitialized)
    {
	if (IsActive())
	    Cancel();

	m_resolver.Close();
	m_sockServ.Close();
    }
    
    HX_DELETE(m_pHostname);

    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pAPManager);

    if (m_pAPResponse)
    {
	m_pAPResponse->ClearPointers();
    }
    HX_RELEASE(m_pAPResponse);
}

    /*
     *  IUnknown methods
     */
STDMETHODIMP HXSymbianResolver::QueryInterface(THIS_
					REFIID riid,
					void** ppvObj)
{
    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXResolver*)this },
	{ GET_IIDHANDLE(IID_IHXResolver), (IHXResolver*) this },
    };	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXSymbianResolver::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)HXSymbianResolver::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}

    /*
     *	IHXResolver methods
     */

STDMETHODIMP HXSymbianResolver::Init(THIS_ IHXResolverResponse*  pResponse)
{
    DPRINTF(D_RESOLVER, ("HXSymbianResolver::Init()\n"));

    HX_RELEASE(m_pResponse);
    
    m_pResponse = pResponse;

    if (m_pResponse)
	m_pResponse->AddRef();

    return (m_bInitialized) ? HXR_OK : HXR_FAILED;
}

STDMETHODIMP HXSymbianResolver::GetHostByName(THIS_ const char* pHostName)
{
    DPRINTF(D_RESOLVER, ("HXSymbianResolver::GetHostByName(%s)\n", pHostName));

    HX_RESULT res = HXR_FAILED;

    DECLARE_SMART_POINTER_UNKNOWN scopeRef((IHXResolver*)this);

    if (m_bInitialized && pHostName && !m_pHostname)
    {
	TInt length = strlen(pHostName);

	m_pHostname = HBufC::NewMax(length);
	
	if (m_pHostname)
	{
	    for (TInt i = 0; i < length; i++)
	    {
		m_pHostname->Des()[i] = pHostName[i];
	    }

	    if (m_pAPManager && m_pAPResponse)
	    {
		res = m_pAPManager->Connect(m_pAPResponse);
	    }
	    else
	    {
		// We don't have an access point manager so
		// we should just simulate it's call on the
		// response object.
		APConnectDone(HXR_OK);
		res = HXR_OK;
	    }
	}
	else
	{
	    res = HXR_OUTOFMEMORY;
	}
    }

    return res;
}

void HXSymbianResolver::RunL()
{
    DPRINTF(D_RESOLVER, ("HXSymbianResolver::RunL()\n"));
    ULONG32 ulAddr = 0;
    HX_RESULT status = HXR_DNR;
    HXBOOL bDoneResolving = TRUE;

    DECLARE_SMART_POINTER_UNKNOWN scopeRef((IHXResolver*)this);

    if (iStatus == KErrNone)
    {
	TInetAddr ipAddr(m_nameEntry().iAddr);

	ulAddr = (ULONG32)ipAddr.Address();
	status = HXR_OK;
    }
    else if ((iStatus == KErrTimedOut) && (m_ulRetryCount > 0))
    {
	// Decrement retry count and try again

	m_ulRetryCount--;

	StartResolve();

	bDoneResolving = FALSE;
    }
    
    if (bDoneResolving)
    {
	DispatchResponse(status, ulAddr);
    }
}

void HXSymbianResolver::DoCancel()
{}

void HXSymbianResolver::static_APConnectDone(void* pObj, HX_RESULT status)
{
    HXSymbianResolver* pResolv = (HXSymbianResolver*)pObj;

    if (pResolv)
    {
	pResolv->APConnectDone(status);
    }
}
void HXSymbianResolver::APConnectDone(HX_RESULT status)
{
    DPRINTF(D_RESOLVER, ("HXSymbianResolver::APConnectDone(%08x)\n",
			 status));
    if (HXR_OK == status)
    {
	m_ulRetryCount = DefaultRetryCount;
	StartResolve();
    }
    else
    {
	DispatchResponse(status, 0);
    }
}

void HXSymbianResolver::StartResolve()
{
    DPRINTF(D_RESOLVER, ("HXSymbianResolver::StartResolve()\n"));
    m_resolver.GetByName(*m_pHostname, m_nameEntry, iStatus);
    SetActive();
}

void HXSymbianResolver::DispatchResponse(HX_RESULT status, ULONG32 ulAddr)
{
    DPRINTF(D_RESOLVER, ("HXSymbianResolver::DispatchResponse(%08x, %08x)\n",
			 status, ulAddr));
    HX_DELETE(m_pHostname);
    
    if (m_pResponse)
    {
	m_pResponse->GetHostByNameDone(status, ulAddr);
    }
}
