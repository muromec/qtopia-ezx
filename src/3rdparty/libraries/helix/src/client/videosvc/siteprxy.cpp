/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: siteprxy.cpp,v 1.6 2007/07/06 21:58:49 jfinnecy Exp $
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
#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxmap.h"
#include "hxwin.h"
#include "chxxtype.h"
#include "ihxpckts.h"
#include "hxslist.h"
#include "hxstrutl.h"
#include "sitemgr.h"
#include "siteprxy.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

/*
 * CHXSiteUserSupplierProxy methods
 */

/************************************************************************
 *  Method:
 *    Constructor
 */
CHXSiteUserSupplierProxy::CHXSiteUserSupplierProxy
(
    CHXSiteManager* pSiteMgr,
    IHXSiteUserSupplier* pSUS,
    const char* pRegionName
)
    : m_lRefCount(0)
    , m_pSiteMgr(pSiteMgr)
    , m_pSUS(pSUS)
    , m_pRegionName(NULL)
{
    HX_ASSERT(m_pSiteMgr);
    HX_ASSERT(m_pSUS);
    HX_ASSERT(pRegionName);

    m_pSiteMgr->AddRef();
    m_pSUS->AddRef();
    m_pRegionName = new_string(pRegionName);
}

/************************************************************************
 *  Method:
 *    Destructor
 */
CHXSiteUserSupplierProxy::~CHXSiteUserSupplierProxy()
{
    HX_RELEASE(m_pSiteMgr);
    HX_RELEASE(m_pSUS);
    HX_VECTOR_DELETE(m_pRegionName);
}

/************************************************************************
 *  Method:
 *    IUnknown::QueryInterface
 */
STDMETHODIMP 
CHXSiteUserSupplierProxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXSiteUserSupplier))
    {
	AddRef();
	*ppvObj = (IHXSiteUserSupplier*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }

    return m_pSUS->QueryInterface(riid, ppvObj);
}

/************************************************************************
 *  Method:
 *    IUnknown::AddRef
 */
STDMETHODIMP_(ULONG32) 
CHXSiteUserSupplierProxy::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/************************************************************************
 *  Method:
 *    IUnknown::Release
 */
STDMETHODIMP_(ULONG32) 
CHXSiteUserSupplierProxy::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *  Method:
 *    IHXSiteUserSupplier::CreateSiteUser
 */
STDMETHODIMP 
CHXSiteUserSupplierProxy::CreateSiteUser
(
    REF(IHXSiteUser*) pSiteUser
)
{
    HX_RESULT rc = m_pSUS->CreateSiteUser(pSiteUser);
    if(HXR_OK == rc)
    {
	CHXSiteUserProxy* pProxy = 
	    new CHXSiteUserProxy(m_pSiteMgr, pSiteUser, m_pRegionName);	

	if (pProxy)
	{
	    HX_RELEASE(pSiteUser);

	    rc = pProxy->QueryInterface(IID_IHXSiteUser, (void**)&pSiteUser);	
	}
    }
    return rc;
}

/************************************************************************
 *  Method:
 *    IHXSiteUserSupplier::DestroySiteUser
 */
STDMETHODIMP 
CHXSiteUserSupplierProxy::DestroySiteUser
(
    IHXSiteUser* pSiteUser
)
{
    return m_pSUS->DestroySiteUser(pSiteUser);
}


/************************************************************************
 *  Method:
 *    IHXSiteUserSupplier::NeedsWindowedSites
 */
STDMETHODIMP_(HXBOOL) 
CHXSiteUserSupplierProxy::NeedsWindowedSites()
{
    return m_pSUS->NeedsWindowedSites();
}


/*
 * CHXSiteUserProxy methods
 */

/************************************************************************
 *  Method:
 *    Constructor
 */
CHXSiteUserProxy::CHXSiteUserProxy
(
    CHXSiteManager* pSiteMgr,
    IHXSiteUser* pSU,
    const char* pRegionName
)
    : m_lRefCount(0)
    , m_pSiteMgr(pSiteMgr)
    , m_pSU(pSU)
    , m_pSite(NULL)
    , m_pRegionName(NULL)
{
    HX_ASSERT(m_pSiteMgr);
    HX_ASSERT(pRegionName);
    HX_ASSERT(m_pSU);

    m_pSU->AddRef();
    m_pSiteMgr->AddRef();
    m_pRegionName = new_string(pRegionName);
}

/************************************************************************
 *  Method:
 *    Destructor
 */
CHXSiteUserProxy::~CHXSiteUserProxy()
{
    HX_RELEASE(m_pSU);
    HX_RELEASE(m_pSiteMgr);
    HX_VECTOR_DELETE(m_pRegionName);
}

/************************************************************************
 *  Method:
 *    IUnknown::QueryInterface
 */
STDMETHODIMP 
CHXSiteUserProxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXSiteUser))
    {
	AddRef();
	*ppvObj = (IHXSiteUser*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }

    return m_pSU->QueryInterface(riid, ppvObj);
}

/************************************************************************
 *  Method:
 *    IUnknown::AddRef
 */
STDMETHODIMP_(ULONG32) 
CHXSiteUserProxy::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/************************************************************************
 *  Method:
 *    IUnknown::Release
 */
STDMETHODIMP_(ULONG32) 
CHXSiteUserProxy::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *  Method:
 *    IHXSiteUser::AttachSite
 */
STDMETHODIMP 
CHXSiteUserProxy::AttachSite
(
    IHXSite* pSite
)
{
    m_pSite = pSite;
    m_pSite->AddRef();

    m_pSiteMgr->HookedSiteAdded(
	m_pRegionName, m_pSite);
    HX_RESULT rc = m_pSU->AttachSite(pSite);
    return rc;
}

/************************************************************************
 *  Method:
 *    IHXSiteUser::DetachSite
 */
STDMETHODIMP 
CHXSiteUserProxy::DetachSite
(
)
{
    HX_RESULT rc = m_pSU->DetachSite();
    m_pSiteMgr->HookedSiteRemoved(
	m_pRegionName, m_pSite);
    HX_RELEASE(m_pSite);
    return rc;
}

/************************************************************************
 *  Method:
 *    IHXSiteUser::HandleEvent
 */
STDMETHODIMP 
CHXSiteUserProxy::HandleEvent
(
    HXxEvent* pEvent
)
{
    // Assume that event has not been handled
    // if it gets here - set the event OUT
    // values in case somebody forgot to...

    pEvent->result = HXR_OK;
    pEvent->handled = FALSE;

    HX_RESULT rc = m_pSiteMgr->HandleHookedEvent(
	m_pRegionName, m_pSite, pEvent);
    if(HXR_OK == rc && !pEvent->handled)
    {
	rc = m_pSU->HandleEvent(pEvent);
    }
    return rc;
}


/************************************************************************
 *  Method:
 *    IHXSiteUser::NeedsWindowedSites
 */
STDMETHODIMP_(HXBOOL) 
CHXSiteUserProxy::NeedsWindowedSites()
{
    return m_pSU->NeedsWindowedSites();
}
