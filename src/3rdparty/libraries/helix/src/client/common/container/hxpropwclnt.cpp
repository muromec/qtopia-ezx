/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpropwclnt.cpp,v 1.10 2006/02/16 23:07:04 ping Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#include "hlxclib/stdio.h"
#include "hlxclib/string.h"

#include "debug.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "db_misc.h"
#include "watchlst.h"
#include "property.h"
#include "commreg.h"
#include "pckunpck.h"
#include "hxmon.h"
#include "watchlst.h"
#include "hxclreg.h"
#include "hxpropwclnt.h"
#include "hxslist.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXClientPropWatch::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
HXClientPropWatch::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXPropWatch), (IHXPropWatch*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPropWatch*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXClientPropWatch::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
HXClientPropWatch::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXClientPropWatch::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
HXClientPropWatch::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}   

HXClientPropWatch::HXClientPropWatch(CommonRegistry* pRegistry, IUnknown* pContext)
	   : m_lRefCount(0)
	   , m_pResponse(NULL)
	   , m_pRegistry(NULL)
	   , m_pInterruptSafeResponse(NULL)
	   , m_pInterruptState(NULL)
	   , m_pScheduler(NULL)
	   , m_pInternalResponse(NULL)
	   , m_pCallback(NULL)
	   , m_pContext(pContext)
{
    HX_ADDREF(m_pContext);

    m_pRegistry = pRegistry;

    /* 
     * Users of HXClientRegistry should call Init and the context should
     * expose IHXInterruptState and IHXScheduler if they want to
     * ensure that watches are fired ONLY at non-interrupt time.
     *
     * Currently, this functionality is ONLY used by the client core
     * since it is multi-threaded and needs to deal with
     * top level clients which are not thread-safe.
     */
    if (pContext)
    {
    	pContext->QueryInterface(IID_IHXScheduler, 
					(void**) &m_pScheduler);

	pContext->QueryInterface(IID_IHXInterruptState, 
					(void**) &m_pInterruptState);

    }

    m_pInternalResponse = new PropWatchResponse(this);
    m_pInternalResponse->AddRef();
}

HXClientPropWatch::~HXClientPropWatch()
{
    if (m_pCallback &&
	m_pCallback->m_bIsCallbackPending &&
	m_pScheduler)
    {
	m_pCallback->m_bIsCallbackPending = FALSE;
	m_pScheduler->Remove(m_pCallback->m_PendingHandle);
    }

    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pInterruptSafeResponse);
    HX_RELEASE(m_pInterruptState);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pInternalResponse);
    HX_RELEASE(m_pCallback);
    HX_RELEASE(m_pContext);
}

/*
 *  Function Name:  	HXClientPropWatch::Init
 *  Input Params:   	IHXPropWatchResponse* pResponse, 
 *  Return Value:   	STDMETHODIMP
 *  Description:
 *      Initialize with the response object and the registry so that
 *  Watch notifications can be sent back to the respective plugins.
 */
STDMETHODIMP
HXClientPropWatch::Init(IHXPropWatchResponse* pResponse)
{
    if (pResponse)
    {
	m_pResponse = pResponse;
	m_pResponse->AddRef();

	m_pResponse->QueryInterface(IID_IHXInterruptSafe, 
					(void**)&m_pInterruptSafeResponse);
	return HXR_OK;
    }

    return HXR_FAIL;
}

/*
 *  Function Name:  	SetWatchOnRoot
 *  Input Params:
 *  Return Value:   	UINT32
 *  Description:
 *  	set a watch point at the root of the registry hierarchy.
 *  to be notified if any property at this level gets added/modified/deleted.
 */
STDMETHODIMP_(UINT32)
HXClientPropWatch::SetWatchOnRoot()
{
    PropWatch* pPropWatch = new PropWatch;
    pPropWatch->m_pResponse = m_pInternalResponse;
    
    return m_pRegistry->SetWatch(pPropWatch);
}

/*
 *  Function Name:  	SetWatchByName
 *  Input Params:   	const char* prop_name
 *  Return Value:   	UINT32
 *  Description:
 *  	set a watch point on any Property. if the Property gets
 *  modified/deleted a notification will be sent by the registry.
 */
STDMETHODIMP_(UINT32)
HXClientPropWatch::SetWatchByName(const char* prop_name)
{
    UINT32 uiret = 0;
    PropWatch* pPropWatch = new PropWatch;
    pPropWatch->m_pResponse = m_pInternalResponse;
    
    uiret = m_pRegistry->SetWatch(prop_name, pPropWatch);

    if ( uiret == 0 )
    {
        //Failed to add watch.
        HX_DELETE( pPropWatch );
    }

    return uiret;
}

/*
 *  Function Name:  	SetWatchById
 *  Input Params:   	const UINT32 id
 *  Return Value:   	UINT32
 *  Description:
 *  	set a watch point on any Property. if the Property gets
 *  modified/deleted a notification will be sent by the registry.
 */
STDMETHODIMP_(UINT32)
HXClientPropWatch::SetWatchById(const UINT32 id)
{
    UINT32 uiret = 0;
    PropWatch* pPropWatch = new PropWatch;
    pPropWatch->m_pResponse = m_pInternalResponse;

    uiret = m_pRegistry->SetWatch(id, pPropWatch);

    if ( uiret == 0 )
    {
        //Failed to add watch.
        HX_DELETE( pPropWatch );
    }

    return uiret;
}

/*
 *  Function Name:  	ClearWatchOnRoot
 *  Input Params:
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	clear a watch point from the root of the DB hierarchy
 */
STDMETHODIMP
HXClientPropWatch::ClearWatchOnRoot()
{
    return m_pRegistry->ClearWatch(m_pInternalResponse);
}

/*
 *  Function Name:  	ClearWatchByName
 *  Input Params:   	const char* prop_name
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	clear a watch point on a property.
 */
STDMETHODIMP
HXClientPropWatch::ClearWatchByName(const char* prop_name)
{
    return m_pRegistry->ClearWatch(prop_name, m_pInternalResponse);
}

/*
 *  Function Name:  	ClearWatchById
 *  Input Params:   	const UINT32 id
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	clear a watch point on a property.
 */
STDMETHODIMP
HXClientPropWatch::ClearWatchById(const UINT32 id)
{
    return m_pRegistry->ClearWatch(id, m_pInternalResponse);
}


// PropWatchCallback

HXClientPropWatch::PropWatchCallback::PropWatchCallback(HXClientPropWatch* pClientPropWatch) :
     m_pPropWatch(pClientPropWatch)
    ,m_PendingHandle (0)
    ,m_bIsCallbackPending (FALSE)
    ,m_lRefCount (0)
{
}

HXClientPropWatch::PropWatchCallback::~PropWatchCallback()
{
}

/*
 * IUnknown methods
 */

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your
//              object.
//
STDMETHODIMP HXClientPropWatch::PropWatchCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXCallback*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::AddRef
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) HXClientPropWatch::PropWatchCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::Release
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) HXClientPropWatch::PropWatchCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}


/*
 *      IHXCallback methods
 */
STDMETHODIMP HXClientPropWatch::PropWatchCallback::Func(void)
{
    m_PendingHandle         = 0;
    m_bIsCallbackPending    = FALSE;

    if (m_pPropWatch && m_pPropWatch->m_pInternalResponse)
    {
	m_pPropWatch->m_pInternalResponse->ProcessPendingResponses();
	m_pPropWatch->Release();
    }

    return HXR_OK;
}


// PropWatchResponse

HXClientPropWatch::PropWatchResponse::PropWatchResponse(HXClientPropWatch* pClientPropWatch) :
     m_pPropWatch(pClientPropWatch)
    ,m_lRefCount(0)
    ,m_pPendingResponseList(NULL)
    ,m_pMutex(NULL)
{
    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pPropWatch->m_pContext);
}

HXClientPropWatch::PropWatchResponse::~PropWatchResponse()
{
    m_pMutex->Lock();
    
    while (m_pPendingResponseList && m_pPendingResponseList->GetCount() > 0)
    {
	PropResponseValues* pValues = (PropResponseValues*) 
					m_pPendingResponseList->RemoveHead();
	delete pValues;
    }

    HX_DELETE(m_pPendingResponseList);
    
    m_pMutex->Unlock();
    HX_RELEASE(m_pMutex);
}

/*
 * IUnknown methods
 */

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your
//              object.
//
STDMETHODIMP HXClientPropWatch::PropWatchResponse::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXPropWatchResponse), (IHXPropWatchResponse*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPropWatchResponse*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::AddRef
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) HXClientPropWatch::PropWatchResponse::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::Release
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) HXClientPropWatch::PropWatchResponse::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}


/*
 *	IHXPropWatchResponse methods
 */
STDMETHODIMP
HXClientPropWatch::PropWatchResponse::AddedProp(const UINT32		id,
			     const HXPropType   	propType,
			     const UINT32		ulParentID)
{
    /* If we are at interrupt time, make sure that the 
     * response is interrupt safe
     */
    if (m_pPropWatch->m_pInterruptState &&
	m_pPropWatch->m_pInterruptState->AtInterruptTime() &&
	(!m_pPropWatch->m_pInterruptSafeResponse ||
	!m_pPropWatch->m_pInterruptSafeResponse->IsInterruptSafe()))
    {
	ScheduleCallback(ADDEDPROP, id, propType, ulParentID);
    }
    else
    {
	ProcessPendingResponses();
	m_pPropWatch->m_pResponse->AddedProp(id, propType, ulParentID);
    }

    return HXR_OK;
}

STDMETHODIMP
HXClientPropWatch::PropWatchResponse::ModifiedProp(const UINT32		id,
			     const HXPropType   	propType,
			     const UINT32		ulParentID)
{
    /* If we are at interrupt time, make sure that the 
     * response is interrupt safe
     */
    if (m_pPropWatch->m_pInterruptState &&
	m_pPropWatch->m_pInterruptState->AtInterruptTime() &&
	(!m_pPropWatch->m_pInterruptSafeResponse ||
	!m_pPropWatch->m_pInterruptSafeResponse->IsInterruptSafe()))
    {
	ScheduleCallback(MODIFIEDPROP, id, propType, ulParentID);
    }
    else
    {
	ProcessPendingResponses();
	m_pPropWatch->m_pResponse->ModifiedProp(id, propType, ulParentID);
    }

    return HXR_OK;
}

STDMETHODIMP
HXClientPropWatch::PropWatchResponse::DeletedProp(const UINT32		id,
			     const UINT32		ulParentID)
{
    /* If we are at interrupt time, make sure that the 
     * response is interrupt safe
     */
    if (m_pPropWatch->m_pInterruptState &&
	m_pPropWatch->m_pInterruptState->AtInterruptTime() &&
	(!m_pPropWatch->m_pInterruptSafeResponse ||
	!m_pPropWatch->m_pInterruptSafeResponse->IsInterruptSafe()))
    {
	ScheduleCallback(DELETEDPROP, id, (HXPropType) 0, ulParentID);
    }
    else
    {
	ProcessPendingResponses();
	m_pPropWatch->m_pResponse->DeletedProp(id, ulParentID);
    }

    return HXR_OK;
}

void
HXClientPropWatch::PropWatchResponse::ScheduleCallback(ResponseType	uResponseType,
				    const UINT32		id,
				    const HXPropType   	propType,
				    const UINT32		ulParentID)
{
    m_pMutex->Lock();
    
    if (!m_pPendingResponseList)
    {
	m_pPendingResponseList = new CHXSimpleList;
    }

    PropResponseValues* pValues = new PropResponseValues(uResponseType, id, 
						    propType, ulParentID);

    m_pPendingResponseList->AddTail((void*) pValues);

    if (!m_pPropWatch->m_pCallback)
    {
	m_pPropWatch->m_pCallback = new PropWatchCallback(m_pPropWatch);
	m_pPropWatch->m_pCallback->AddRef();
    }

    if (!m_pPropWatch->m_pCallback->m_bIsCallbackPending &&
	m_pPropWatch->m_pScheduler)
    {
	m_pPropWatch->AddRef();
	
	m_pPropWatch->m_pCallback->m_bIsCallbackPending = TRUE;
	m_pPropWatch->m_pCallback->m_PendingHandle = 
	    m_pPropWatch->m_pScheduler->RelativeEnter(
				    m_pPropWatch->m_pCallback, 0);
    }
    
    m_pMutex->Unlock();
}

void
HXClientPropWatch::PropWatchResponse::ProcessPendingResponses()
{
    /* remove any pending callback */
    if (m_pPropWatch->m_pCallback &&
	m_pPropWatch->m_pCallback->m_bIsCallbackPending &&
	m_pPropWatch->m_pScheduler)
    {
	m_pPropWatch->m_pCallback->m_bIsCallbackPending = FALSE;
	m_pPropWatch->m_pScheduler->Remove(
			m_pPropWatch->m_pCallback->m_PendingHandle);
    }

    m_pMutex->Lock();
    
    while (m_pPendingResponseList && m_pPendingResponseList->GetCount() > 0)
    {
	PropResponseValues* pValues = (PropResponseValues*) 
					m_pPendingResponseList->RemoveHead();

	switch (pValues->m_uResponseType)
	{
	    case ADDEDPROP:
		m_pPropWatch->m_pResponse->AddedProp(pValues->m_ulId, 
						     pValues->m_propType,
						     pValues->m_ulParentID);
    		break;
	    case MODIFIEDPROP:
		m_pPropWatch->m_pResponse->ModifiedProp(pValues->m_ulId, 
						      pValues->m_propType,
						      pValues->m_ulParentID);
		break;
	    case DELETEDPROP:
		m_pPropWatch->m_pResponse->DeletedProp(pValues->m_ulId, 
						       pValues->m_ulParentID);
		break;
	    default:
		HX_ASSERT(0);
		break;
	}

	delete pValues;
    }
    
    m_pMutex->Unlock();
}
