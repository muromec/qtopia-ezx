/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sitemgr.cpp,v 1.13 2007/07/06 21:58:49 jfinnecy Exp $
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

/****************************************************************************
 * 
 *  Basic PN implementation of the IHXSite classes.
 *
 *
 */
#include "hxcom.h"
#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxmap.h"
#include "hxwin.h"
#include "hxplgns.h"
#include "hxengin.h"
#include "chxxtype.h"
#include "hxsite2.h"
#include "ihxpckts.h"
#include "hxslist.h"
#include "sitemgr.h"
#include "siteprxy.h"
#include "hxvctrl.h"
#include "hxvsurf.h"
#ifndef _WINCE
#ifdef _WINDOWS
#ifdef _WIN32
#include <vfw.h>
#else
#include <drawdib.h>
#endif /* _WIN32 */
#endif /* _WINDOWS */
#if defined(_MACINTOSH)
#include "maclibrary.h"
#include "hx_moreprocesses.h"
#endif
#endif

#include "hxstrutl.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if !defined(HELIX_CONFIG_NOSTATICS)
INT32			CHXSiteManager::zm_nSiteManagerCount = 0;
#else
#include "globals/hxglobals.h"
const INT32		CHXSiteManager::zm_nSiteManagerCount = 0;
#endif

#if HELIX_FEATURE_SITEMANAGER_EVENTHANDLING
#define HX_ENABLE_SITE_EVENTHANDLER 1
CHXSimpleList CHXSiteManager::zm_SiteManagerList;
#endif	// HELIX_FEATURE_SITEMANAGER_EVENTHANDLING

#ifdef _MACINTOSH

HXBOOL 	CHXSiteManager::zm_bWindowRemovedFromList = FALSE;

#endif


/************************************************************************
 *  Method:
 *    Destructor
 */
CHXSiteManager::CHXSiteManager(IUnknown* pContext)
    : m_lRefCount(0)
    , m_bInUnHookAll(FALSE)
    , m_bNeedFocus(FALSE)
    , m_pContext(pContext)
{
#if defined(HELIX_CONFIG_NOSTATICS)
    //INT32& zm_nSiteManagerCount = HXGlobalInt32::Get(&CHXSiteManager::zm_nSiteManagerCount);
#endif

    HX_ADDREF(m_pContext);
    
#if defined(HX_ENABLE_SITE_EVENTHANDLER)
    zm_SiteManagerList.AddTail(this);
    zm_nSiteManagerCount++;
#endif /* HX_ENABLE_SITE_EVENTHANDLER */
};

/************************************************************************
 *  Method:
 *    Destructor
 */
CHXSiteManager::~CHXSiteManager()
{
#if defined(HELIX_CONFIG_NOSTATICS)
    //INT32& zm_nSiteManagerCount = HXGlobalInt32::Get(&CHXSiteManager::zm_nSiteManagerCount);
#endif

#if defined(HX_ENABLE_SITE_EVENTHANDLER)
    LISTPOSITION pos = zm_SiteManagerList.Find(this);
    zm_SiteManagerList.RemoveAt(pos);
    
    zm_nSiteManagerCount--;
#endif /* HX_ENABLE_SITE_EVENTHANDLER */

    HX_ASSERT(m_MasterListOfSites.IsEmpty());
    HX_ASSERT(m_SitesToSUS.IsEmpty());

    RemoveMapStrToObj(&m_ChannelsToLists);
    RemoveMapStrToObj(&m_PersistentChannelsToLists);
    RemoveMapStrToObj(&m_LSGNamesToLists);
    RemoveMapStrToObj(&m_PersistentLSGNamesToLists);

    CHXMapStringToOb::Iterator ndx = m_EventHookMap.Begin();
    for (; ndx != m_EventHookMap.End(); ++ndx)
    {
        RemoveList((CHXSimpleList*)*ndx);
	delete (CHXSimpleList*)*ndx;
    }
    m_EventHookMap.RemoveAll();

    RemoveList(&m_UnnamedEventHookList);

    CleanupPendingValues();
    
    HX_RELEASE(m_pContext);
}

HX_RESULT   
CHXSiteManager::ProcessSiteEvent(CHXEventHookElement* pElement, IHXSite* pSite, 
                                 HXxEvent* pEvent, EVENT_TYPE event_type)
{
    HX_RESULT   rc = HXR_OK;

    if (pElement)
    {
        switch (event_type)
        {
        case SITE_EVENT_GENERAL:
	    rc = pElement->m_pHook->HandleEvent(pSite, pEvent);
            break;
        case SITE_EVENT_REMOVED:
            rc = pElement->m_pHook->SiteRemoved(pSite);
            break;
        case SITE_EVENT_ADDED:
            rc = pElement->m_pHook->SiteAdded(pSite);
            break;
        default:
            HX_ASSERT(FALSE);
            break;
        }
    }

    return rc;
}


HX_RESULT
CHXSiteManager::HandleSiteEvent(const char* pRegionName, IHXSite* pSite, 
                                HXxEvent* pEvent, EVENT_TYPE event_type)
{
    HX_RESULT   rc = HXR_OK;

    // first, walk through named hooks
    CHXSimpleList* pList = NULL;
    if (m_EventHookMap.Lookup(pRegionName, (void*&)pList))
    {
	CHXSimpleList::Iterator iter = pList->Begin();
	for (; iter != pList->End(); ++iter)
	{
	    CHXEventHookElement* pElement =
		(CHXEventHookElement*)(*iter);
            
            rc = ProcessSiteEvent(pElement, pSite, pEvent, event_type);
	    if (pEvent && pEvent->handled)
	    {
		break;
	    }
	}
    }

    // try the null hooks
    if (pEvent && !pEvent->handled)
    {
	CHXSimpleList::Iterator iter = m_UnnamedEventHookList.Begin();
	for (; iter != m_UnnamedEventHookList.End(); ++iter)
	{
	    CHXEventHookElement* pElement =
		(CHXEventHookElement*)(*iter);

            rc = ProcessSiteEvent(pElement, pSite, pEvent, event_type);
	    if (pEvent && pEvent->handled)
	    {
		break;
	    }
	}
    }

    return rc;
}

void 
CHXSiteManager::HookupHelper(CHXMapPtrToPtr* pMap, char* pActualString, HXBOOL bIsPersistent, 
                             PTR_TYPE ptr_type, HOOK_TYPE hook_type)
{
    if (pMap)
    {
        CHXMapPtrToPtr::Iterator ndx = pMap->Begin();
        for (; ndx != pMap->End(); ++ndx)
        {
	    IHXBuffer* ptempValues = (IHXBuffer*)ndx.get_key(); 
            if (HOOKUP_BY_LSGNAMEWITHSTRING == hook_type)
            {
                HookupByLSGNameWithString((IHXSiteUserSupplier*) *ndx, 
                                          (char*)(ptempValues->GetBuffer()), 
                                          bIsPersistent);
            }
            else if ( !strcasecmp((char*)(ptempValues->GetBuffer()), pActualString ) ) 
	    {
                switch (hook_type)
                {
                case HOOKUP_BY_PLAYTOFROMWITHSTRING:
                    HookupByPlayToFromWithString((IHXSiteUserSupplier*) *ndx,
                                                 (char*)(ptempValues->GetBuffer()), 
                                                 bIsPersistent);
                    break;
                case HOOKUP_SINGLESITE_BY_LSGNAMEWITHSTRING:
		    HookupSingleSiteByLSGNameWithString((IHXSiteUser*) *ndx, 
                                                        (char*)(ptempValues->GetBuffer()), 
                                                        bIsPersistent);
                    break;
                case HOOKUP_SINGLESITE_BY_PLAYTOFROMWITHSTRING:
                    HookupSingleSiteByPlayToFromWithString((IHXSiteUser*) *ndx,
                                                           (char*)(ptempValues->GetBuffer()), 
                                                           bIsPersistent);

                    break;
                default:
                    HX_ASSERT(FALSE);
                    break;
                }
	    }
        }
    }
}

void 
CHXSiteManager::RemoveMapStrToObj(CHXMapStringToOb* pMap)
{
    if (pMap)
    {
        CHXMapStringToOb::Iterator ndx = pMap->Begin();
        for (; ndx != pMap->End(); ++ndx)
        {
	    CHXMapPtrToPtr* pNode = (CHXMapPtrToPtr*)(*ndx);
	    HX_ASSERT(pNode->IsEmpty());
	    delete pNode;
        }
        pMap->RemoveAll();
    }
}

void 
CHXSiteManager::RemoveMapPtrToPtr(CHXMapPtrToPtr* pMap)
{
    if (pMap)
    {
        CHXMapPtrToPtr::Iterator i;
        for(i = pMap->Begin(); i!= pMap->End();++i)
        {
            IHXBuffer* pValue = (IHXBuffer*)i.get_key();
	    CHXSiteUserSupplierProxy* pProxy = (CHXSiteUserSupplierProxy*) *i; 
	    HX_RELEASE(pValue);
	    HX_RELEASE(pProxy);
        }
        pMap->RemoveAll();
    }
}

void 
CHXSiteManager::RemoveList(CHXSimpleList* pList)
{
    if (pList)
    {
        CHXSimpleList::Iterator iter = pList->Begin();
        for (; iter != pList->End(); ++iter)
        {
	    CHXEventHookElement* pElement = (CHXEventHookElement*)(*iter);
	    delete pElement;
        }
        pList->RemoveAll();
    }
}

void CHXSiteManager::CleanupPendingValues()
{
    RemoveMapPtrToPtr(&m_PendingValueToSULSG);
    RemoveMapPtrToPtr(&m_PendingValueToSUSingleLSG);
    RemoveMapPtrToPtr(&m_PendingValueToSUPlayTo);
    RemoveMapPtrToPtr(&m_PendingValueToSUSinglePlayTo);
}

/************************************************************************
 *  Method:
 *    IUnknown::QueryInterface
 */
STDMETHODIMP 
CHXSiteManager::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXSiteManager), (IHXSiteManager*)this },
            { GET_IIDHANDLE(IID_IHXEventHookMgr), (IHXEventHookMgr*)this },
            { GET_IIDHANDLE(IID_IHXSiteManager2), (IHXSiteManager2*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXSiteManager*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/************************************************************************
 *  Method:
 *    IUnknown::AddRef
 */
STDMETHODIMP_(ULONG32) 
CHXSiteManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/************************************************************************
 *  Method:
 *    IUnknown::Release
 */
STDMETHODIMP_(ULONG32) 
CHXSiteManager::Release()
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
 *    IHXSiteManager::AddSiteByStringHelper
 */
STDMETHODIMP 
CHXSiteManager::AddSiteByStringHelper
(
    const char*		pString,
    IHXSite*		pSite,
    CHXMapStringToOb&	ByStringMap
)
{
    HRESULT	    hresFinal	    = HXR_OK;
    void*	    pVoid	    = NULL;
    CHXMapPtrToPtr* pSiteCollection = NULL;

    /*
     * The basic data structure is as follows: A map is kept 
     * from names to "lists" of sites. We don't actually use
     * a list because we want to quickly detect if the item is
     * already in the list so we use a map for the sites as well.
     */

    /*
     * Find the list in the name to list map, if there is no
     * list in the map then we need to create a new list object
     * and add it to the map.
     */
    if (!ByStringMap.Lookup(pString,pVoid))
    {
	pSiteCollection = new CHXMapPtrToPtr();
	ByStringMap.SetAt(pString,pSiteCollection);
    }
    else
    {
	pSiteCollection = (CHXMapPtrToPtr*)pVoid;
    }
    /*
     * Now that we have the collection of sites we want to see
     * if this item is already in the collection. If it is, then
     * we barf out an error!
     */
    if (pSiteCollection->Lookup(pSite,pVoid))
    {
	hresFinal = HXR_INVALID_PARAMETER;
	goto exit;
    }
    /*
     * Now that we know its cool to add this to the collection,
     * we do just that...
     */
    pSiteCollection->SetAt(pSite,pSite);

    /*
     * We also want to keep track of which collection we added
     * the site to so that we can more easily (quickly) delete
     * the site when it is removed. To do this we keep a master
     * map of pSite's to the pSiteList items.
     *
     * NOTE: This site should not already be in the master 
     * list. This debug check is a sanity check since our
     * previous efforts should have returned out as an error
     * in the event that a site was added twice!
     */
    HX_ASSERT(!m_MasterListOfSites.Lookup(pSite,pVoid));
    m_MasterListOfSites[pSite] = pSiteCollection;

exit:

    return hresFinal;
}


/************************************************************************
 *  Method:
 *    IHXSiteManager::AddSite
 */
STDMETHODIMP 
CHXSiteManager::AddSite(IHXSite* pSite)
{
    IHXValues*	    pProps = NULL;
    IHXBuffer*	    pValue = NULL;
    char*	    pActualString = NULL;
    HRESULT	    hresTemp;
    HRESULT	    hresFinal = HXR_OK;
    HXBOOL	    bIsPersistent = FALSE;

    if (m_pContext)
    {
	// Give sites the same context as the site manager to allow sites to obtain
	// services specific to the environment in which they are managed (e.g. IHXCientEngine).
	IHXObjectConfiguration* pObjectConfiguration = NULL;
	if (HXR_OK == pSite->QueryInterface(IID_IHXObjectConfiguration, (void**) &pObjectConfiguration))
	{
	    pObjectConfiguration->SetContext(m_pContext);
	}
	HX_RELEASE(pObjectConfiguration);
    }
    
    /*
     * We need to get the IHXValues for the site so we know it
     * its for by LSGName or for by plattofrom use. If this is not
     * available then barf up an error.
     */
    hresTemp = pSite->QueryInterface(IID_IHXValues,(void**)&pProps);
    if (HXR_OK != hresTemp)
    {
	hresFinal = hresTemp;
	goto exit;
    }

    /*
     * let's see if this is a persistent site...
     */
    hresTemp = pProps->GetPropertyCString("Persistent",pValue);
    if(HXR_OK == hresTemp)
    {
	bIsPersistent = TRUE;
	HX_RELEASE(pValue);
    }

    /*
     * Now let's determine if it's by LSGName or by playtofrom.
     * If one of these is not available then barf up an error.
     */

    /*
     * If the "LayoutGroup" property exists than this is 
     * a site for layout groups by LSGName.
     */
    hresTemp = pProps->GetPropertyCString("LayoutGroup",pValue);
    if (HXR_OK == hresTemp)
    {
	pActualString = (char*)pValue->GetBuffer();
	if(bIsPersistent)
	{
	    hresFinal = AddSiteByStringHelper(pActualString, pSite, m_PersistentLSGNamesToLists);
	}
	else
	{
	    hresFinal = AddSiteByStringHelper(pActualString, pSite, m_LSGNamesToLists);
	}
	goto exit;
    }

    /*
     * If the "channel" property exists than this is 
     * a site for renderers playing to a channel.
     */
    hresTemp = pProps->GetPropertyCString("channel",pValue);
    if (HXR_OK == hresTemp)
    {
	pActualString = (char*)pValue->GetBuffer();
	if(bIsPersistent)
	{
	    hresFinal = AddSiteByStringHelper(pActualString, pSite, m_PersistentChannelsToLists);
	}
	else
	{
	    hresFinal = AddSiteByStringHelper(pActualString, pSite, m_ChannelsToLists);
	}
	goto exit;
    }

#if 0 ////// NOT YET SUPPORTED ////////
    /*
     * If the "playfrom" property exists than this is 
     * a site for renderers playing from a source/stream combo.
     * Notice that more properties than just "playfrom" are needed
     * to do the actual hookup so we pass the properties in as well.
     */
    hresTemp = pProps->GetPropertyCString("playfrom",pValue);
    if (HXR_OK == hresTemp)
    {
	hresFinal = AddSiteByPlayFrom(pProperties,pSite);
	goto exit;
    }
#endif

exit:
    /*
     * Cleanup any temporary objects....
     */
    HX_RELEASE(pProps);
    HX_RELEASE(pValue);

    // hookup any leftover orphan value/sites (pending)...

    // first process the LSG list	       
    HookupHelper(&m_PendingValueToSULSG, pActualString, bIsPersistent, 
                 SITE_USER_SUPPLIER, HOOKUP_BY_LSGNAMEWITHSTRING);

    // next process the Single LSG list
    HookupHelper(&m_PendingValueToSUSingleLSG, pActualString, bIsPersistent, 
                 SITE_USER, HOOKUP_SINGLESITE_BY_LSGNAMEWITHSTRING);
    
    // next process play to list
    HookupHelper(&m_PendingValueToSUPlayTo, pActualString, bIsPersistent, 
                 SITE_USER_SUPPLIER, HOOKUP_BY_PLAYTOFROMWITHSTRING);

    // next process the Single LSG list
    HookupHelper(&m_PendingValueToSUSinglePlayTo, pActualString, bIsPersistent, 
                 SITE_USER, HOOKUP_SINGLESITE_BY_PLAYTOFROMWITHSTRING);

#ifdef _WINDOWS
    if (m_bNeedFocus && pSite)
    {
	IHXSiteWindowless* pWindowLess = NULL;
	IHXSiteWindowed*   pWindowed	= NULL;

	pSite->QueryInterface(IID_IHXSiteWindowless, (void**) &pWindowLess);
	if (pWindowLess)
	{
	    pWindowLess->QueryInterface(IID_IHXSiteWindowed, (void**) &pWindowed);
	}

	if (pWindowed)
	{
	    HXxWindow* pWindow = pWindowed->GetWindow();
	    if (pWindow && pWindow->window)
	    {
		// same logic exists in pnvideo/win/winsite.cpp: _SetFocus()
		HWND hTmp = ::GetForegroundWindow();
		if( ::IsChild(hTmp, (HWND)pWindow->window ))
		{
		    ::SetFocus((HWND)pWindow->window);
		}
	    }
	}

	HX_RELEASE(pWindowLess);
	HX_RELEASE(pWindowed);
    }
#endif

    return hresFinal;
}

/************************************************************************
 *  Method:`
 *    IHXSiteManager::RemoveSite
 */
STDMETHODIMP 
CHXSiteManager::RemoveSite(IHXSite* pSite)
{
    void*	    pVoid	    = NULL;
    CHXMapPtrToPtr* pSiteCollection = NULL;
    IHXValues*	    pProps	    = NULL;
    IHXBuffer*	    pValue	    = NULL;
    HXBOOL	    bIsPersistent   = FALSE;

    /*
     * This site must have been previously added and therefore
     * should be in the master list of sites.
     */
    if (!m_MasterListOfSites.Lookup(pSite,pVoid)) 
    {
	return HXR_INVALID_PARAMETER;
    }

    /*
     * determine whether the site is persistent, and get the name of
     * the site so it can be removed from the channel list
     */
    if(HXR_OK == pSite->QueryInterface(IID_IHXValues,(void**)&pProps))
    {
	if(HXR_OK == pProps->GetPropertyCString("Persistent", pValue))
	{
	    bIsPersistent = TRUE;
	    HX_RELEASE(pValue);
	}
	HX_RELEASE(pProps);
    }

    /*
     *
     */

    /* If we are unhooking all sites, we do not want to unhook site here */
    if (!m_bInUnHookAll)
    {
	UnhookSite(pSite, bIsPersistent);
    }

    /*
     * We need to remove the site from whatever collection of
     * sites it is in. This means we are removing it from the
     * collection of sites for it's LSGName if that is how it
     * was added. Instead of determining the properties supported
     * by the site to determine which collection to remove it from
     * we stored the site collection in our master list. Cool, eh?
     */
    pSiteCollection = (CHXMapPtrToPtr*)pVoid;
    HX_ASSERT(pSiteCollection->Lookup(pSite,pVoid));
    pSiteCollection->RemoveKey(pSite);

    /*
     * Of course we also need to remove it from the master site
     * list as well.
     */
    m_MasterListOfSites.RemoveKey(pSite);

    return HXR_OK;
}


STDMETHODIMP
CHXSiteManager::AddEventHookElement
(
    CHXSimpleList*	    pList,
    CHXEventHookElement*    pHookElement
)
{
    // insert element into pList
    // in region/uLayer order (greater uLayer
    // before lesser uLayer)

    HX_RESULT rc = HXR_OK;

    HXBOOL bInserted = FALSE;

    LISTPOSITION pos = pList->GetHeadPosition();
    while(pos)
    {
	CHXEventHookElement* pElement = 
	    (CHXEventHookElement*)pList->GetAt(pos);
	if(pElement->m_uLayer <= pHookElement->m_uLayer)
	{
	    pList->InsertBefore(pos, pHookElement);
	    bInserted = TRUE;
	    break;
	}
	pList->GetNext(pos);
    }
    if(!bInserted)
    {
	pList->AddTail(pHookElement);
    }

    return rc;
}

STDMETHODIMP
CHXSiteManager::RemoveEventHookElement
(
    CHXSimpleList* pList,
    IHXEventHook* pHook,
    UINT16 uLayer
)
{
    HX_RESULT rc = HXR_OK;

    LISTPOSITION pos = pList->GetHeadPosition();
    while(pos)
    {
	CHXEventHookElement* pThisElement = 
	    (CHXEventHookElement*)pList->GetAt(pos);
	if(pHook == pThisElement->m_pHook && 
	   uLayer == pThisElement->m_uLayer)
	{
	    delete pThisElement;
	    pList->RemoveAt(pos);
	    break;
	}
	pList->GetNext(pos);
    }
    return rc;
}

/************************************************************************
 *  Method:
 *    IHXEventHookMgr::AddHook
 */
STDMETHODIMP 
CHXSiteManager::AddHook
(
    IHXEventHook* 	pHook,
    const char* 	pRegionName,
    UINT16 		uLayer
)
{
    HX_RESULT rc = HXR_OK;

    CHXEventHookElement* pElement = 
	new CHXEventHookElement(pHook, uLayer);

    if(pRegionName && strlen(pRegionName) > 0)
    {
	CHXSimpleList* pList = NULL;
	if (!m_EventHookMap.Lookup(pRegionName, (void*&)pList))
	{
	    pList = new CHXSimpleList;
	    m_EventHookMap[pRegionName] = pList;
	}
	rc = AddEventHookElement(pList, pElement);
    }
    else
    {
	rc = AddEventHookElement(&m_UnnamedEventHookList, pElement);
    }

    return rc;
}

/************************************************************************
 *  Method:
 *    IHXEventHookMgr::RemoveHook
 */
STDMETHODIMP 
CHXSiteManager::RemoveHook
(
    IHXEventHook* 	pHook,
    const char* 	pRegionName,
    UINT16 		uLayer
)
{
    HX_RESULT rc = HXR_OK;

    if(pRegionName && strlen(pRegionName) > 0)
    {
	CHXSimpleList* pList = NULL;
	if (m_EventHookMap.Lookup(pRegionName, (void*&)pList))
	{
	    rc = RemoveEventHookElement(pList, pHook, uLayer);
	}
    }
    else
    {
	rc = RemoveEventHookElement(&m_UnnamedEventHookList, pHook, uLayer);
    }

    return rc;
}

STDMETHODIMP CHXSiteManager::GetNumberOfSites(REF(UINT32) nNumSites )
{
    nNumSites = m_MasterListOfSites.GetCount();
    return HXR_OK;
}

STDMETHODIMP CHXSiteManager::GetSiteAt(UINT32 nIndex, REF(IHXSite*) pSite)
{
    if (!m_MasterListOfSites.GetCount())
    {
        return HXR_FAIL;
    }
    
    POSITION pos = m_MasterListOfSites.GetStartPosition();
    void*   pValue;

    for (UINT32 i = 0; i<=nIndex; i++)
    {
        m_MasterListOfSites.GetNextAssoc(pos, (void*&)pSite, pValue);
    }
    return HXR_OK;
}

/************************************************************************
*	Method:
*	    CHXSiteManager::HandleHookedEvent
*	Purpose:
*	  Pass the event to interested parties
*/
HX_RESULT
CHXSiteManager::HandleHookedEvent
(
    const char* pRegionName,
    IHXSite* pSite,
    HXxEvent* pEvent
)
{
    return HandleSiteEvent(pRegionName, pSite, pEvent, SITE_EVENT_GENERAL);
}

/************************************************************************
*	Method:
*	    CHXSiteManager::HookedSiteAdded
*	Purpose:
*	  Let hooks know about the added site
*/
void
CHXSiteManager::HookedSiteAdded
(
    const char* pRegionName,
    IHXSite* pSite
)
{
    HandleSiteEvent(pRegionName, pSite, NULL, SITE_EVENT_ADDED);
}

/************************************************************************
*	Method:
*	    CHXSiteManager::HookedSiteRemoved
*	Purpose:
*	  Let hooks know about the removed site
*/
void
CHXSiteManager::HookedSiteRemoved
(
    const char* pRegionName,
    IHXSite* pSite
)
{
    HandleSiteEvent(pRegionName, pSite, NULL, SITE_EVENT_REMOVED);
}

/************************************************************************
*	Method:
*	    CHXSiteManager::IsSitePresent
*	Purpose:
*	  
*/
HXBOOL
CHXSiteManager::IsSitePresent
(
    IHXSite*		    pSite
)
{
    void*   pVoid = NULL;
    return m_MasterListOfSites.Lookup(pSite,pVoid);
}

void 
CHXSiteManager::NeedFocus(HXBOOL bNeedFocus)
{
    m_bNeedFocus = bNeedFocus;
}

/************************************************************************
 *  Method:
 *    IHXSiteManager::IsSiteAvailableByStringHelper
 */
HXBOOL
CHXSiteManager::IsSiteAvailableByStringHelper
(
    const char*		pString,
    CHXMapStringToOb&	ByStringMap
)
{
    HXBOOL	    bAvailable	    = FALSE;
    void*	    pVoid	    = NULL;
    CHXMapPtrToPtr* pSiteCollection = NULL;

    /*
     * The basic data structure is as follows: A map is kept 
     * from names to "lists" of sites. We don't actually use
     * a list because we want to quickly detect if the item is
     * already in the list so we use a map for the sites as well.
     */

    /*
     * Find the list in the name to list map, if there is no
     * list in the map then we know it is surely not available.
     */
    if (!ByStringMap.Lookup(pString,pVoid))
    {
	bAvailable = FALSE;
	goto exit;
    }

    pSiteCollection = (CHXMapPtrToPtr*)pVoid;

    /*
     * Now that we have the collection of sites we want to see
     * if there are any items in the collection. The collection
     * may exist with no items in it.
     */
    if (pSiteCollection->IsEmpty())
    {
	bAvailable = FALSE;
	goto exit;
    }

    /*
     * That's all we need to know, if we get this far then we
     * know there exists at least one site by this string.
     */
    bAvailable = TRUE;

exit:

    return bAvailable;
}

/************************************************************************
 *  Method:
 *    CHXSiteManager::IsSiteAvailableByLSGName
 */
HXBOOL
CHXSiteManager::IsSiteAvailableByLSGName(IHXValues* pProps, HXBOOL bIsPersistent)
{
    IHXBuffer*	    pValue = NULL;
    char*	    pActualString = NULL;
    HRESULT	    hresTemp;
    HXBOOL	    bAvailable = FALSE;

    /*
     * The properties passed here are the properties of the
     * site user not the site. When associating by LSG name
     * the site's "LayoutGroup" property must match the site
     * users "name" property. So we get the "name" property
     * from the props and look it in our LSGName list.
     */
    hresTemp = pProps->GetPropertyCString("name",pValue);
    HX_ASSERT(HXR_OK == hresTemp);
    pActualString = (char*)pValue->GetBuffer();
    if(bIsPersistent)
    {
	bAvailable = IsSiteAvailableByStringHelper(pActualString,m_PersistentLSGNamesToLists);
    }
    else
    {
	bAvailable = IsSiteAvailableByStringHelper(pActualString,m_LSGNamesToLists);
    }
    pValue->Release();
    return bAvailable;
}

/************************************************************************
 *  Method:
 *    CHXSiteManager::IsSiteAvailableByPlayToFrom
 */
HXBOOL
CHXSiteManager::IsSiteAvailableByPlayToFrom(IHXValues* pProps, HXBOOL bIsPersistent)
{
    IHXBuffer*	    pValue = NULL;
    char*	    pActualString = NULL;
    HRESULT	    hresTemp;
    HXBOOL	    bAvailable = FALSE;

    /*
     * The properties passed here are the properties of the
     * site user not the site. When associating by PlayToFrom
     * the site's "channel" property must match the site
     * users "playto" property. So we get the "playto" property
     * from the props and look it in our Channel list.
     */
    hresTemp = pProps->GetPropertyCString("playto",pValue);
    HX_ASSERT(HXR_OK == hresTemp);
    pActualString = (char*)pValue->GetBuffer();
    if(bIsPersistent)
    {
	bAvailable = IsSiteAvailableByStringHelper(pActualString,m_PersistentChannelsToLists);
    }
    else
    {
	bAvailable = IsSiteAvailableByStringHelper(pActualString,m_ChannelsToLists);
    }
    pValue->Release();
    return bAvailable;
}

/************************************************************************
 *  Method:
 *    IHXSiteManager::HookupByStringHelper
 */
HXBOOL
CHXSiteManager::HookupByStringHelper
(
    const char*		    pString,
    CHXMapStringToOb&	    ByStringMap,
    IHXSiteUserSupplier*   pSUS,
    HXBOOL		    bIsPersistent
)
{
    void*	    pVoid	    = NULL;
    CHXMapPtrToPtr* pSiteCollection = NULL;
    CHXMapPtrToPtr::Iterator ndxSite;

    /*
     * The basic data structure is as follows: A map is kept 
     * from names to "lists" of sites. We don't actually use
     * a list because we want to quickly detect if the item is
     * already in the list so we use a map for the sites as well.
     */

    /*
     * Find the list in the name to list map, if there is no
     * list in the map then we know it is surely not available.
     */
    if (!ByStringMap.Lookup(pString,pVoid))
    {
	return FALSE;
    }

    /*
     * Now that we have the collection of sites we want to actually
     * hook the site user supplier up to each site in the collection.
     */
    pSiteCollection = (CHXMapPtrToPtr*)pVoid;

    ndxSite = pSiteCollection->Begin();

    for (; ndxSite != pSiteCollection->End(); ++ndxSite)
    {
	IHXSite* pSite	    = (IHXSite*)(*ndxSite);
	IHXSiteUser* pUser = NULL;
	
	// If the site has a user then we should not attempt to attach him
	// further, if he has a user it probably means that we have 
	// already attached him and we are in Wall of TVs mode.
	if (pSite->GetUser(pUser) != HXR_OK || !pUser)
	{
	    HookupSite2SUS(pSite, pSUS, bIsPersistent);
	}

	HX_RELEASE(pUser);
    }

    return TRUE;
}


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\.
////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//.
/************************************************************************
 *  Method:
 *    IHXSiteManager::HookupSite2SUS
 */
HXBOOL
CHXSiteManager::HookupSite2SUS
(
    IHXSite*		    pSite, 
    IHXSiteUserSupplier*   pSUS,
    HXBOOL		    bIsPersistent
)
{
    HXBOOL		    retVal	    = TRUE;
    void*		    pVoid	    = NULL;
    IHXSiteWindowed*	    pSiteWindowed   = NULL;
    IHXSiteUser*	    pUser	    = NULL;
    HXBOOL		    bWindowed	    = FALSE;
    HXBOOL		    bNeedsWindowed  = FALSE;

    /*
     * We need to find out if this site user needs windowed
     * sites. The first step is to determine if this is a
     * windowed site.
     */
    bWindowed = (HXR_OK == pSite->QueryInterface(
					IID_IHXSiteWindowed,
					(void**)&pSiteWindowed));

    HX_RELEASE(pSiteWindowed);

    /*
     * If the site user supplier needs windowed sites and this
     * site is windowed, or if the site user supplier can handle
     * windowless sites, then proceed.
     */
    bNeedsWindowed = pSUS->NeedsWindowedSites();
    if (!bNeedsWindowed || (bNeedsWindowed && bWindowed))
    {

	/*
	 * Ask the site user supplier to create a new site user
	 * for this particular site.
	 */
	if (HXR_OK == pSUS->CreateSiteUser(pUser))
	{
	    /*
	     * Now actually hook up the site to the site user!
	     *
	     * NOTE: The IHXSite is responsible for calling the
	     *	     IHXSiteUser::AttachSite() method.
	     *
	     * NOTE: If this is a layout site user than it will
	     *	     create child site in response to the AttachSite()
	     *	     method call.
	     */
	    pSite->AttachUser(pUser);

	    /*
	     * We also record the site's that we have created users for
	     * here so that we can unhook everything the next time the
	     * layout is changed. We record the site user supplier
	     * that created the site user so that we can delete that user
	     * as well. NOTE: we shouldn't have already created this
	     * user!
	     */
	    if(bIsPersistent)
	    {
		pVoid = NULL;
		HX_ASSERT(!m_PersistentSitesToSUS.Lookup(pSite,pVoid));
		m_PersistentSitesToSUS[pSite] = pSUS;
	    }
	    else
	    {
		HX_ASSERT(!m_SitesToSUS.Lookup(pSite,pVoid));
		m_SitesToSUS[pSite] = pSUS;
	    }
	    pSite->AddRef();
	    pSUS->AddRef();

	    pUser->Release();
	}
	else
	{
	    retVal = FALSE;
	}
    }
    else
    {
	retVal = FALSE;
    }
    return retVal;	
}


/************************************************************************
 *  Method:
 *    CHXSiteManager::HookupByLSGNameWithString
 */
HXBOOL 
CHXSiteManager::HookupByLSGNameWithString
(
    IHXSiteUserSupplier*   pSUS, 
    char*		    pActualString,
    HXBOOL		    bIsPersistent
)
{
    HXBOOL res = FALSE;
    if(bIsPersistent)
    {
	res = HookupByStringHelper(pActualString,m_PersistentLSGNamesToLists,pSUS, bIsPersistent);
	if(!res)
	{
	    // check for existence of string in non-persistent list, then move it if it exists
	    void* pVoid = NULL;
	    if(m_LSGNamesToLists.Lookup(pActualString, pVoid))
	    {
		m_LSGNamesToLists.RemoveKey(pActualString);
		m_PersistentLSGNamesToLists.SetAt(pActualString, pVoid);
		res = HookupByStringHelper(pActualString,m_PersistentLSGNamesToLists,pSUS, bIsPersistent);
	    }
	}
    }
    else
    {
	res = HookupByStringHelper(pActualString,m_LSGNamesToLists,pSUS, bIsPersistent);
    }
    return res;
}

/************************************************************************
 *  Method:
 *    CHXSiteManager::HookupByLSGName
 */
HXBOOL 
CHXSiteManager::HookupByLSGName
(
    IHXSiteUserSupplier*   pSUS, 
    IHXValues*		    pProps,
    HXBOOL		    bIsPersistent
)
{
    IHXBuffer*	    pValue = NULL;
    char*	    pActualString = NULL;
    HRESULT	    hresTemp;

    /*
     * This method is responsible for fully hooking up the
     * site user supplier with any and all sites associate with
     * it's LSG name. NOTE: The Properties passed in are those
     * of this site user supplier, so first thing is to map from
     * the appropriate property to the collection of sites having
     * that property. This is the same as the process for determining
     * availability.
     */
    hresTemp = pProps->GetPropertyCString("name",pValue);
    HX_ASSERT(HXR_OK == hresTemp);
    pActualString = (char*)pValue->GetBuffer();

    CHXSiteUserSupplierProxy* pProxy = new CHXSiteUserSupplierProxy(this,
	pSUS, pActualString);
    pProxy->AddRef();

    HXBOOL res = HookupByLSGNameWithString(pProxy, pActualString, bIsPersistent);
    // when it fails, save the pProxy & pValue for pending hookup
    // which is done later during AddSite()
    if (!res)
    {
	m_PendingValueToSULSG.SetAt(pValue, pProxy);
    }
    else
    {
        pValue->Release();
	pProxy->Release();	// now owned by the hookup list
    }

    return res;
}

/************************************************************************
 *  Method:
 *    CHXSiteManager::HookupByPlayToFromWithSting
 */
HXBOOL
CHXSiteManager::HookupByPlayToFromWithString
(
    IHXSiteUserSupplier*   pSUS, 
    char*		    pActualString,
    HXBOOL		    bIsPersistent
)
{
    HXBOOL res = FALSE;
    if(bIsPersistent)
    {
	res = HookupByStringHelper(pActualString,m_PersistentChannelsToLists,pSUS,bIsPersistent);
	if(!res)
	{
	    // check for existence of string in non-persistent list, then move it if it exists
	    void* pVoid = NULL;
	    if(m_ChannelsToLists.Lookup(pActualString, pVoid))
	    {
		m_ChannelsToLists.RemoveKey(pActualString);
		m_PersistentChannelsToLists.SetAt(pActualString, pVoid);
		res = HookupByStringHelper(pActualString,m_PersistentChannelsToLists,pSUS,bIsPersistent);
	    }
	}
    }
    else
    {
	res = HookupByStringHelper(pActualString,m_ChannelsToLists,pSUS,bIsPersistent);
    }
    return res;
}


/************************************************************************
 *  Method:
 *    CHXSiteManager::HookupByPlayToFrom
 */
HXBOOL
CHXSiteManager::HookupByPlayToFrom
(
    IHXSiteUserSupplier*   pSUS, 
    IHXValues*		    pProps,
    HXBOOL		    bIsPersistent
)
{
    IHXBuffer*	    pValue = NULL;
    char*	    pActualString = NULL;
    HRESULT	    hresTemp;

    /*
     * This method is responsible for fully hooking up the
     * site user supplier with any and all sites associate with
     * it's playto/from infor. NOTE: The Properties passed in 
     * are those of this site user supplier, so first thing is 
     * to map from the appropriate property to the collection 
     * of sites having that property. This is the same as the 
     * process for determining availability.
     */
    hresTemp = pProps->GetPropertyCString("playto",pValue);
    HX_ASSERT(HXR_OK == hresTemp);
    pActualString = (char*)pValue->GetBuffer();

    CHXSiteUserSupplierProxy* pProxy = new CHXSiteUserSupplierProxy(this,
	pSUS, pActualString);
    pProxy->AddRef();

    HXBOOL res = HookupByPlayToFromWithString(pProxy, pActualString, bIsPersistent);

    // when it fails, save the pSU & pValue for pending hookup
    // which is done later during AddSite()
    if (!res)
    {
	m_PendingValueToSUPlayTo.SetAt(pValue, pProxy);
    }
    else
    {
        pValue->Release();
	pProxy->Release();	// now owned by hookup list
    }

    return res;
}

void
CHXSiteManager::RemoveSitesByLSGName(IHXValues* pProps, HXBOOL bIsPersistent)
{
    IHXBuffer* pValue = 0;
    HX_RESULT rc = pProps->GetPropertyCString("name", pValue);
    if(HXR_OK == rc)
    {
	const char* pActualString = (const char*)pValue->GetBuffer();
	if(bIsPersistent)
	{
	    void* pVoid;
	    if(m_PersistentLSGNamesToLists.Lookup(pActualString, pVoid))
	    {
		/*
		 * Now that we have the collection of sites we want to actually
		 * hook the site user supplier up to each site in the collection.
		 */
		CHXMapPtrToPtr* pSiteCollection = (CHXMapPtrToPtr*)pVoid;
		CHXMapPtrToPtr::Iterator ndxSite = pSiteCollection->Begin();

		for (; ndxSite != pSiteCollection->End(); ++ndxSite)
		{
		    IHXSite* pSite = (IHXSite*)(*ndxSite);
		    RemoveSite(pSite);
		}

		delete pSiteCollection;
		m_PersistentLSGNamesToLists.RemoveKey(pActualString);
	    }
	}
	pValue->Release();
    }
}


/************************************************************************
 *  Method:
 *    CHXSiteManager::UnhookSite
 */
void 
CHXSiteManager::UnhookSite(IHXSite* pSite, HXBOOL bIsPersistent)
{
    HX_ASSERT(pSite);
    /*
     * To unhook all the items we simple run through the site users
     * we created, ask them for their site, tell the site to detach
     * from the user, and tell the site user supplier to destroy the
     * site user. That's simple enough...
     */

    IHXSiteUserSupplier*	pSUS  = 0;
    IHXSiteUser*		pUser = 0;

    /* Check in both */
    if(m_PersistentSitesToSUS.Lookup(pSite, (void*&)pSUS))
    {
	m_PersistentSitesToSUS.RemoveKey(pSite);
	pSite->GetUser(pUser);
	pSite->DetachUser();
	if(pUser && pSUS)
	{
	    pSUS->DestroySiteUser(pUser);
	    pSUS->Release();
	}
	
	HX_RELEASE(pUser);
	HX_RELEASE(pSite);
    }
    else if(m_SitesToSUS.Lookup(pSite, (void*&)pSUS))
    {
	m_SitesToSUS.RemoveKey(pSite);
	pSite->GetUser(pUser);
	pSite->DetachUser();
	if(pUser && pSUS)
	{
	    pSUS->DestroySiteUser(pUser);
	    pSUS->Release();
	}
	HX_RELEASE(pUser);
	HX_RELEASE(pSite);
    }
}

/************************************************************************
 *  Method:
 *    CHXSiteManager::UnhookAll
 */
void 
CHXSiteManager::UnhookAll()
{
     m_bInUnHookAll = TRUE;

    /*
     * To unhook all the items we simple run through the site users
     * we created, ask them for their site, tell the site to detach
     * from the user, and tell the site user supplier to destroy the
     * site user. That's simple enough...
     */
    CHXMapPtrToPtr::Iterator ndxSite = m_SitesToSUS.Begin();

    for (;  ndxSite != m_SitesToSUS.End(); ++ndxSite)
    {
	IHXSite*		pSite = (IHXSite*)ndxSite.get_key();
	IHXSiteUserSupplier*	pSUS  = (IHXSiteUserSupplier*)(*ndxSite);
	IHXSiteUser*		pUser = NULL;

	pSite->GetUser(pUser);
	/*
	 * Now actually unhook the site to the site user!
	 *
	 * NOTE: The IHXSite is responsible for calling the
	 *	     IHXSiteUser::DetachSite() method.
	 *
	 * NOTE: If this is a layout site user than it will
	 *	     destroy the child sites in responses to the
	 *	     DetachSite() method call.
	 */
	pSite->DetachUser();

	if (pSUS)
	{
	    if (pUser)
	    {
		pSUS->DestroySiteUser(pUser);
	    }

	    pSUS->Release();
	}
	
	HX_RELEASE(pUser);
	HX_RELEASE(pSite);
    }
    m_SitesToSUS.RemoveAll();

    CleanupPendingValues();
    m_bInUnHookAll = FALSE;
}


/************************************************************************
 *  Method:
 *    CHXSiteManager::HookupSingleSiteByLSGNameWithString
 */
HXBOOL 
CHXSiteManager::HookupSingleSiteByLSGNameWithString
(
    IHXSiteUser*	    pSU,
    char*		    pActualString,
    HXBOOL		    bIsPersistent
)
{
    HXBOOL res = FALSE;
    
    if(bIsPersistent)
    {
	res = HookupSingleSiteByStringHelper(pActualString,m_PersistentLSGNamesToLists,pSU,bIsPersistent);
	if(!res)
	{
	    // check for existence of string in non-persistent list, then move it if it exists
	    void* pVoid = NULL;
	    if(m_LSGNamesToLists.Lookup(pActualString, pVoid))
	    {
		m_LSGNamesToLists.RemoveKey(pActualString);
		m_PersistentLSGNamesToLists.SetAt(pActualString, pVoid);
		res = HookupSingleSiteByStringHelper(pActualString,m_PersistentLSGNamesToLists,
		    pSU,bIsPersistent);
	    }
	}
    }
    else
    {
	res = HookupSingleSiteByStringHelper(pActualString,m_LSGNamesToLists,pSU,bIsPersistent);
    }
    return res;
}


/************************************************************************
 *  Method:
 *    CHXSiteManager::HookupSingleSiteByLSGName
 */
HXBOOL 
CHXSiteManager::HookupSingleSiteByLSGName
(
    IHXSiteUser*	    pSU,
    IHXValues*		    pProps,
    HXBOOL		    bIsPersistent
)
{
    IHXBuffer*	    pValue = NULL;
    char*	    pActualString = NULL;
    HRESULT	    hresTemp;

    /*
     * This method is responsible for fully hooking up the
     * site user supplier with any and all sites associate with
     * it's LSG name. NOTE: The Properties passed in are those
     * of this site user supplier, so first thing is to map from
     * the appropriate property to the collection of sites having
     * that property. This is the same as the process for determining
     * availability.
     */
    hresTemp = pProps->GetPropertyCString("name",pValue);
    HX_ASSERT(HXR_OK == hresTemp);
    pActualString = (char*)pValue->GetBuffer();

    CHXSiteUserProxy* pProxy = new CHXSiteUserProxy(this,
	pSU, pActualString);
    pProxy->AddRef();

    HXBOOL res = HookupSingleSiteByLSGNameWithString(pProxy, pActualString, bIsPersistent);

    // when it fails, save the pProxy & pValue for pending hookup
    // which is done later during AddSite()
    if (!res)
    {
	m_PendingValueToSUSingleLSG.SetAt(pValue, pProxy);
    }
    else
    {
        pValue->Release();
	pProxy->Release();	// now owned by hookup list
    }

    return res;
}


/************************************************************************
 *  Method:
 *    CHXSiteManager::HookupSingleSiteByPlayToFromWithString
 */
HXBOOL
CHXSiteManager::HookupSingleSiteByPlayToFromWithString
(
    IHXSiteUser*	    pSU,
    char*		    pActualString,
    HXBOOL		    bIsPersistent
)
{
    HXBOOL res = FALSE;
    if(bIsPersistent)
    {
	res = HookupSingleSiteByStringHelper(pActualString,m_PersistentChannelsToLists,pSU,bIsPersistent);
	if(!res)
	{
	    // check for existence of string in non-persistent list, then move it if it exists
	    void* pVoid = NULL;
	    if(m_ChannelsToLists.Lookup(pActualString, pVoid))
	    {
		m_ChannelsToLists.RemoveKey(pActualString);
		m_PersistentChannelsToLists.SetAt(pActualString, pVoid);
		res = HookupSingleSiteByStringHelper(pActualString,m_PersistentChannelsToLists,
		    pSU,bIsPersistent);
	    }
	}
    }
    else
    {
	res = HookupSingleSiteByStringHelper(pActualString,m_ChannelsToLists,pSU,bIsPersistent);
    }
    return res;
}

//////////////////////
// JEB: Add ret value 

/************************************************************************
 *  Method:
 *    CHXSiteManager::HookupSingleSiteByPlayToFrom
 */
HXBOOL
CHXSiteManager::HookupSingleSiteByPlayToFrom
(
    IHXSiteUser*	    pSU,
    IHXValues*		    pProps,
    HXBOOL		    bIsPersistent
)
{
    IHXBuffer*	    pValue = NULL;
    char*	    pActualString = NULL;
    HRESULT	    hresTemp;

    /*
     * The properties passed here are the properties of the
     * site user not the site. When associating by PlayToFrom
     * the site's "channel" property must match the site
     * users "playto" property. So we get the "playto" property
     * from the props and look it in our Channel list.
     */
    hresTemp = pProps->GetPropertyCString("playto",pValue);
    HX_ASSERT(HXR_OK == hresTemp);
    pActualString = (char*)pValue->GetBuffer();

    CHXSiteUserProxy* pProxy = new CHXSiteUserProxy(this,
	pSU, pActualString);
    pProxy->AddRef();

    HXBOOL res = HookupSingleSiteByPlayToFromWithString(pProxy, pActualString, bIsPersistent);
    // when it fails, save the pProxy & pValue for pending hookup
    // which is done later during AddSite()
    if (!res)
    {
	m_PendingValueToSUSinglePlayTo.SetAt(pValue, pProxy);
    }
    else
    {
	pProxy->Release();	// now owned by hookup list
        pValue->Release();
    }

    return res;
}

/************************************************************************
 *  Method:
 *    IHXSiteManager::HookupSingleSiteByStringHelper
 */
HXBOOL
CHXSiteManager::HookupSingleSiteByStringHelper
(
    const char*		    pString,
    CHXMapStringToOb&	    ByStringMap,
    IHXSiteUser*	    pSU,
    HXBOOL		    bIsPersistent
)
{
    void*			pVoid = NULL;
    CHXMapPtrToPtr*		pSiteCollection = NULL;
    CHXMapPtrToPtr::Iterator	ndxSite;
    IHXSite*			pSite = NULL;
    IHXSiteWindowed*		pSiteWindowed = NULL;
    IHXSiteUser*		pUser = pSU;
    HXBOOL			bWindowed = FALSE;
    HXBOOL			bNeedsWindowed = FALSE;

    /*
     * The basic data structure is as follows: A map is kept 
     * from names to "lists" of sites. We don't actually use
     * a list because we want to quickly detect if the item is
     * already in the list so we use a map for the sites as well.
     */

    /*
     * Find the list in the name to list map, if there is no
     * list in the map then we know it is surely not available.
     */
    if (!ByStringMap.Lookup(pString,pVoid))
    {
	/* We may get back site from the site supplier at a later time due to 
	 * asynchronous nature. Make this request pending and we will try 
	 * to HookupSingleSiteByStringHelper from within AddSite()
	 */
	return FALSE;
    }

    /*
     * Now that we have the collection of sites we want to actually
     * hook the site user supplier up to _THE_FIRST_ site in the collection.
     */
    pSiteCollection = (CHXMapPtrToPtr*)pVoid;

    ndxSite = pSiteCollection->Begin();

    pSite = (IHXSite*)(*ndxSite);

    /*
     * We need to find out if this site user needs windowed
     * sites. The first step is to determine if this is a
     * windowed site.
     */
    bWindowed = (HXR_OK == pSite->QueryInterface(
					IID_IHXSiteWindowed,
					(void**)&pSiteWindowed));

    HX_RELEASE(pSiteWindowed);
    
    /*
     * If the site user supplier needs windowed sites and this
     * site is windowed, or if the site user supplier can handle
     * windowless sites, then proceed.
     */
    bNeedsWindowed = pSU->NeedsWindowedSites();
    if (!bNeedsWindowed || (bNeedsWindowed && bWindowed))
    {

	/*
	 * Now actually hook up the site to the site user!
	 *
	 * NOTE: The IHXSite is responsible for calling the
	 *	     IHXSiteUser::AttachSite() method.
	 *
	 * NOTE: If this is a layout site user than it will
	 *	     create child site in response to the AttachSite()
	 *	     method call.
	 */
	pSite->AttachUser(pUser);

	/*
	 * We also record the site's that we have created users for
	 * here so that we can unhook everything the next time the
	 * layout is changed. We record the site user supplier
	 * that created the site user so that we can delete that user
	 * as well. NOTE: we shouldn't have already created this
	 * user!
	 */

	if(bIsPersistent)
	{
	    HX_ASSERT(!m_PersistentSitesToSUS.Lookup(pSite,pVoid));

	    /*
	     * NULL means no Site User supplier was provided.
	     */
	    m_PersistentSitesToSUS[pSite] = NULL;
	}
	else
	{
	    HX_ASSERT(!m_SitesToSUS.Lookup(pSite,pVoid));

	    /*
	     * NULL means no Site User supplier was provided.
	     */
	    m_SitesToSUS[pSite] = NULL;
	}
	pSite->AddRef();
    }

    return TRUE;
}

/************************************************************************
*	Method:
*	    CHXSiteManager::EventOccurred
*	Purpose:
*	  Pass the event to appropriate sites
*/
void 
CHXSiteManager::EventOccurred(HXxEvent* pEvent)
{
#if defined(HX_ENABLE_SITE_EVENTHANDLER)
    
    // xxxbobclark
    // first, try iterating through sites the old-fashioned way. If there are
    // any sites here then they'll be in the zm_SiteWindowedList list. If there
    // aren't, then try using the new pnvideo way.
    
    HXBOOL bHandledOldEvent = FALSE;
    
#if 0    /* XXX BOB PLEASE TAKE A LOOK */
    //determine which sites should get the event & send it over
    CHXSimpleList::Iterator ndxSite = CHXSiteWindowed::zm_SiteWindowedList.Begin();

    for (;  ndxSite != CHXSiteWindowed::zm_SiteWindowedList.End(); ++ndxSite)
    {
	zm_bWindowRemovedFromList = FALSE;

	CHXSiteWindowed* pSiteWindowed = (CHXSiteWindowed*)(*ndxSite);

	bHandledOldEvent = TRUE;
	
	pSiteWindowed->AddRef();
	pSiteWindowed->MacEventOccurred(pEvent);
	pSiteWindowed->Release();


	/* If a window was removed from this list, iterate again
	 * from the head. 
	 * This *hack* is to avoid O(n^2) processing of events. 
	 * It is required since a new URL may be opened from 
	 * within MacEventOccured() call resulting in 
	 * releasing one(or more) CHXSiteWindows.
	 * 
	 * Potential Problem: Same event may be given to the same 
	 * window more than once.
	 */
	if (zm_bWindowRemovedFromList)
	{
	    zm_bWindowRemovedFromList = FALSE;
	    ndxSite = CHXSiteWindowed::zm_SiteWindowedList.Begin();
	}
    }
#endif

    if (!bHandledOldEvent)
    {
	CHXSimpleList::Iterator siteManagerNdx = zm_SiteManagerList.Begin();
	for (; siteManagerNdx != zm_SiteManagerList.End(); ++siteManagerNdx)
	{
	    CHXSiteManager* pSiteManager = (CHXSiteManager*)(*siteManagerNdx);
	    CHXMapPtrToPtr::Iterator ndx = pSiteManager->m_MasterListOfSites.Begin();
	    for (; ndx != pSiteManager->m_MasterListOfSites.End(); ++ndx)
	    {
		IHXSite* pSite = (IHXSite*)ndx.get_key();

		IHXSiteWindowless* pSiteWindowless = NULL;
		if (pSite->QueryInterface(IID_IHXSiteWindowless, (void**)&pSiteWindowless) == HXR_OK)
		{
		    pSiteWindowless->EventOccurred(pEvent);
		    pSiteWindowless->Release();
		}
	    }
	}
    }
#endif /* HX_ENABLE_SITE_EVENTHANDLER */
}

/*
 * CHXEventHookElement methods
 */

CHXEventHookElement::CHXEventHookElement
(
    IHXEventHook*  pHook,
    UINT16	    uLayer
) : m_pHook(pHook)
  , m_uLayer(uLayer)
{
    HX_ASSERT(m_pHook);
    m_pHook->AddRef();
}

CHXEventHookElement::~CHXEventHookElement()
{
    HX_RELEASE(m_pHook);
}

