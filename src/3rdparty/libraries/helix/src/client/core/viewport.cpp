/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: viewport.cpp,v 1.5 2007/07/06 21:58:12 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"

#include "hxengin.h"
#include "hxcore.h"
#include "hxslist.h"
#include "hxmap.h"

#include "hxgroup.h"
#include "hxplay.h"
#include "hxcleng.h"
#include "viewport.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifndef _UNIX /* probably a chinstrap tlc :) */
#define XXXMEH_VIEWPORT_HACK 1
#endif

HXViewPort::HXViewPort(HXViewPortManager*	pViewPortManager,
			 IHXValues*		pValues,
			 const char*		pszName)
			 : m_lRefCount(0)
			 , m_pszName(pszName)
			 , m_pParent(pViewPortManager)
			 , m_pValues(pValues)
{   
    if (m_pValues)
    {
	m_pValues->AddRef();
    }
}

HXViewPort::~HXViewPort()
{
    HX_RELEASE(m_pValues);
}

STDMETHODIMP
HXViewPort::QueryInterface(REFIID riid, void**ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)this },
            { GET_IIDHANDLE(IID_IHXViewPort), (IHXViewPort*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXViewPort::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXViewPort::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *	Method:
 *	    IHXViewPort::GetName
 *	Purpose:
 *	    get name of the viewport
 */
STDMETHODIMP_(const char*)
HXViewPort::GetName()
{
    return m_pszName;
}

/************************************************************************
 *	Method:
 *	    IHXViewPort::GetProperties
 *	Purpose:
 *	    get properties of the viewport
 */
STDMETHODIMP
HXViewPort::GetProperties(REF(IHXValues*)   pValues)
{
    pValues = m_pValues;
    if (pValues)
    {
	pValues->AddRef();
    }

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXViewPort::Show
 *	Purpose:
 *	    show viewport
 */
STDMETHODIMP
HXViewPort::Show()
{
    return m_pParent->OnViewPortShow(m_pszName);
}

/************************************************************************
 *	Method:
 *	    IHXViewPort::Hide
 *	Purpose:
 *	    hide viewport
 */
STDMETHODIMP
HXViewPort::Hide()
{
    return m_pParent->OnViewPortHide(m_pszName);
}

/************************************************************************
 *	Method:
 *	    IHXViewPort::SetFocus
 *	Purpose:
 *	    set focus on viewport
 */
STDMETHODIMP
HXViewPort::SetFocus()
{
    return m_pParent->OnViewPortFocus(m_pszName);
}

/************************************************************************
 *	Method:
 *	    IHXViewPort::SetZOrder
 *	Purpose:
 *	    set Z order on viewport
 */
STDMETHODIMP
HXViewPort::SetZOrder(UINT32	ulZOrder)
{
    return m_pParent->OnViewPortZOrder(m_pszName, ulZOrder);
}

HXViewPortManager::HXViewPortManager(HXPlayer* pPlayer)
			:   m_lRefCount(0)
			,   m_pPlayer(pPlayer)
			,   m_pViewPortMap(NULL)
			,   m_pViewPortSinkList(NULL)
			,   m_pViewPortSupplier(NULL)
{
    if (m_pPlayer)
        m_pPlayer->AddRef();
}

HXViewPortManager::~HXViewPortManager()
{
    Close();
}

STDMETHODIMP
HXViewPortManager::QueryInterface(REFIID riid, void**ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)this },
            { GET_IIDHANDLE(IID_IHXViewPortManager), (IHXViewPortManager*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXViewPortManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXViewPortManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *	Method:
 *	    IHXViewPortManager::OpenViewPort
 *	Purpose:
 *	    create viewport
 */
STDMETHODIMP
HXViewPortManager::OpenViewPort(IHXValues* pValues, IHXSiteUser* pSiteUser)
{
    HX_RESULT		rc = HXR_OK;
    const char*		pszViewPort = NULL;
    IHXBuffer*		pBuffer = NULL;
    HXViewPort*	pViewPort = NULL;
    IHXViewPortSink*	pViewPortSink = NULL;
    CHXSimpleList::Iterator ndx;

    if (!pValues)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    if (HXR_OK != pValues->GetPropertyCString("playto", pBuffer))
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    pszViewPort = (const char*)pBuffer->GetBuffer();

    pViewPort = new HXViewPort(this, pValues, pszViewPort);
    pViewPort->AddRef();

    if (!m_pViewPortMap)  
    {
	m_pViewPortMap = new CHXMapStringToOb();
    }

    m_pViewPortMap->SetAt(pszViewPort, (void*)pViewPort);

#ifdef XXXMEH_VIEWPORT_HACK
    // XXXMEH - hack for now until we get TLC support for
    // IHXViewPortSupplier. If we don't get support from
    // the TLC for IHXViewPortSupplier, then QI the SMIL
    // renderer for it. We also have to check it every time
    // to see if it has changed. This is because the core
    // may change the persistent component and not let us
    // know about it.
    {
        if (m_pViewPortSinkList)
        {
            LISTPOSITION pos = m_pViewPortSinkList->GetHeadPosition();
            while (pos)
            {
                IHXViewPortSink* pSink =
                    (IHXViewPortSink*) m_pViewPortSinkList->GetNext(pos);
                if (pSink)
                {
                    // QI for IHXViewPortSupplier
                    IHXViewPortSupplier* pSupplier = NULL;
                    pSink->QueryInterface(IID_IHXViewPortSupplier, (void**) &pSupplier);
                    if (pSupplier != m_pViewPortSupplier)
                    {
                        HX_RELEASE(m_pViewPortSupplier);
                        m_pViewPortSupplier = pSupplier;
                        m_pViewPortSupplier->AddRef();
                        HX_RELEASE(pSupplier);
                        break;
                    }
                    HX_RELEASE(pSupplier);
                }
            }
        }
    }
#else
    if (m_pPlayer && !m_pViewPortSupplier)
    {
	if (HXR_OK != m_pPlayer->QueryInterface(IID_IHXViewPortSupplier, (void**)&m_pViewPortSupplier))
	{
	    m_pViewPortSupplier = NULL;
	}
    }
#endif

    if (m_pViewPortSupplier)
    {
	m_pViewPortSupplier->OnViewPortOpen(pValues, pSiteUser);

	if (m_pViewPortSinkList)
	{
	    ndx = m_pViewPortSinkList->Begin();
	    for (; ndx != m_pViewPortSinkList->End(); ++ndx)
	    {
		pViewPortSink = (IHXViewPortSink*) (*ndx);
		pViewPortSink->ViewPortOpened(pszViewPort);
	    }
	}
    }

cleanup:

   HX_RELEASE(pBuffer);
   
   return rc;
}

/************************************************************************
 *	Method:
 *	    IHXViewPortManager::GetViewPort
 *	Purpose:
 *	    get viewport
 */
STDMETHODIMP
HXViewPortManager::GetViewPort(const char* pszViewPort,
				REF(IHXViewPort*) pViewPort)
{
    HX_RESULT	rc = HXR_OK;

    pViewPort = NULL;

    if (!m_pViewPortMap)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    if (!m_pViewPortMap->Lookup(pszViewPort, (void*&)pViewPort))
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    pViewPort->AddRef();

cleanup:

    return rc;
}

/************************************************************************
 *	Method:
 *	    IHXViewPortManager::CloseViewPort
 *	Purpose:
 *	    remove viewport
 */
STDMETHODIMP
HXViewPortManager::CloseViewPort(const char* pszViewPort)
{
    HX_RESULT		rc = HXR_OK;
    IHXViewPort*	pViewPort = NULL;
    IHXViewPortSink*	pViewPortSink = NULL;
    CHXSimpleList::Iterator ndx;

    if (m_pViewPortMap &&
	m_pViewPortMap->Lookup(pszViewPort, (void*&)pViewPort))
    {
	m_pViewPortMap->RemoveKey(pszViewPort);
	HX_RELEASE(pViewPort);
    }

    if (m_pViewPortSupplier)
    {
	m_pViewPortSupplier->OnViewPortClose(pszViewPort);

	if (m_pViewPortSinkList)
	{
	    ndx = m_pViewPortSinkList->Begin();
	    for (; ndx != m_pViewPortSinkList->End(); ++ndx)
	    {
		pViewPortSink = (IHXViewPortSink*) (*ndx);
		pViewPortSink->ViewPortClosed(pszViewPort);
	    }
	}
    }

    return rc;
}

/************************************************************************
 *	Method:
 *	    IHXViewPortManager::AddViewPortSink
 *	Purpose:
 *	    add viewport sinker
 */
STDMETHODIMP
HXViewPortManager::AddViewPortSink(IHXViewPortSink*  pViewPortSink)
{
    HX_RESULT	rc = HXR_OK;

    if (!m_pViewPortSinkList)
    {
	m_pViewPortSinkList = new CHXSimpleList();
    }

    m_pViewPortSinkList->AddTail(pViewPortSink);
    pViewPortSink->AddRef();

    return rc;
}

/************************************************************************
 *	Method:
 *	    IHXViewPortManager::RemoveViewPortSink
 *	Purpose:
 *	    remove viewport sinker
 */
STDMETHODIMP
HXViewPortManager::RemoveViewPortSink(IHXViewPortSink*  pViewPortSink)
{
    if (!m_pViewPortSinkList)
    {
	return HXR_UNEXPECTED;
    }

    LISTPOSITION lPosition = m_pViewPortSinkList->Find(pViewPortSink);
    if (!lPosition)
    {
	return HXR_UNEXPECTED;
    }

    m_pViewPortSinkList->RemoveAt(lPosition);
    HX_RELEASE(pViewPortSink);
    
    return HXR_OK;
}


HX_RESULT
HXViewPortManager::OnViewPortShow(const char* pszViewPortName)
{
    HX_RESULT		rc = HXR_OK;
    IHXViewPortSink*	pViewPortSink = NULL;
    CHXSimpleList::Iterator ndx;

    if (!m_pViewPortSupplier)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    m_pViewPortSupplier->OnViewPortShow(pszViewPortName);

    if (m_pViewPortSinkList)
    {
	ndx = m_pViewPortSinkList->Begin();
	for (; ndx != m_pViewPortSinkList->End(); ++ndx)
	{
	    pViewPortSink = (IHXViewPortSink*) (*ndx);
	    pViewPortSink->ViewPortShown(pszViewPortName);
	}
    }

cleanup:

    return rc;
}

HX_RESULT
HXViewPortManager::OnViewPortHide(const char* pszViewPortName)
{
    HX_RESULT		rc = HXR_OK;
    IHXViewPortSink*	pViewPortSink = NULL;
    CHXSimpleList::Iterator ndx;

    if (!m_pViewPortSupplier)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    m_pViewPortSupplier->OnViewPortHide(pszViewPortName);

    if (m_pViewPortSinkList)
    {
	ndx = m_pViewPortSinkList->Begin();
	for (; ndx != m_pViewPortSinkList->End(); ++ndx)
	{
	    pViewPortSink = (IHXViewPortSink*) (*ndx);
	    pViewPortSink->ViewPortHidden(pszViewPortName);
	}
    }

cleanup:

    return rc;
}

HX_RESULT
HXViewPortManager::OnViewPortFocus(const char* pszViewPortName)
{
    HX_RESULT		rc = HXR_OK;
    IHXViewPortSink*	pViewPortSink = NULL;
    CHXSimpleList::Iterator ndx;

    if (!m_pViewPortSupplier)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    m_pViewPortSupplier->OnViewPortFocus(pszViewPortName);

    if (m_pViewPortSinkList)
    {
	ndx = m_pViewPortSinkList->Begin();
	for (; ndx != m_pViewPortSinkList->End(); ++ndx)
	{
	    pViewPortSink = (IHXViewPortSink*) (*ndx);
	    pViewPortSink->ViewPortFocusSet(pszViewPortName);
	}
    }

cleanup:

    return rc;
}

HX_RESULT
HXViewPortManager::OnViewPortZOrder(const char* pszViewPortName, 
				     UINT32 ulZOrder)
{
    HX_RESULT		rc = HXR_OK;
    IHXViewPortSink*	pViewPortSink = NULL;
    CHXSimpleList::Iterator ndx;

    if (!m_pViewPortSupplier)
    {
	rc = HXR_FAILED;
	goto cleanup;
    }

    m_pViewPortSupplier->OnViewPortZOrder(pszViewPortName, ulZOrder);

    if (m_pViewPortSinkList)
    {
	ndx = m_pViewPortSinkList->Begin();
	for (; ndx != m_pViewPortSinkList->End(); ++ndx)
	{
	    pViewPortSink = (IHXViewPortSink*) (*ndx);
	    pViewPortSink->ViewPortZOrderSet(pszViewPortName, ulZOrder);
	}
    }

cleanup:

    return rc;
}

void
HXViewPortManager::Close()
{
    if (m_pViewPortMap)
    {
	CHXMapStringToOb::Iterator ndx = m_pViewPortMap->Begin();
	for (; ndx != m_pViewPortMap->End(); ++ndx)
	{
	    HXViewPort* pViewPort = (HXViewPort*)(*ndx);
	    HX_RELEASE(pViewPort);
	}
    }
    HX_DELETE(m_pViewPortMap);

    if (m_pViewPortSinkList)
    {
	CHXSimpleList::Iterator ndx = m_pViewPortSinkList->Begin();
	for (; ndx != m_pViewPortSinkList->End(); ++ndx)
	{
	    IHXViewPortSink* pViewPortSink = (IHXViewPortSink*)(*ndx);
	    HX_RELEASE(pViewPortSink);
	}
    }
    HX_DELETE(m_pViewPortSinkList);

    HX_RELEASE(m_pViewPortSupplier);
    HX_RELEASE(m_pPlayer);
}

