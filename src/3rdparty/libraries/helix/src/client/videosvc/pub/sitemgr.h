/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sitemgr.h,v 1.10 2007/07/06 21:58:50 jfinnecy Exp $
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

#ifndef _SITEMGR_H_
#define _SITEMGR_H_

struct	IHXSiteUserSupplier;

class	CHXSiteWindowed;

/*
 * element containd in event hook list
 */
class CHXEventHookElement
{
public:
    CHXEventHookElement	    (IHXEventHook* pHook,
			    UINT16	    uLayer);

    ~CHXEventHookElement    ();

    IHXEventHook* m_pHook;
    UINT16	   m_uLayer;
};

/****************************************************************************
 * 
 *  Class:
 *
 *	CHXSiteManager
 *
 *  Purpose:
 *
 *	Implementation for IHXSiteManager in the client core.
 *
 */
class CHXSiteManager : public IHXSiteManager,
		       public IHXEventHookMgr,
                       public IHXSiteManager2
{
private:
    LONG32				m_lRefCount;
    HXBOOL				m_bInUnHookAll;
    HXBOOL				m_bNeedFocus;
    
    IUnknown*				m_pContext;

    CHXMapPtrToPtr			m_MasterListOfSites;

    CHXMapStringToOb			m_ChannelsToLists;
    CHXMapStringToOb			m_LSGNamesToLists;
    CHXMapPtrToPtr			m_SitesToSUS;

    CHXMapStringToOb			m_PersistentChannelsToLists;
    CHXMapStringToOb			m_PersistentLSGNamesToLists;
    CHXMapPtrToPtr			m_PersistentSitesToSUS;

    CHXMapPtrToPtr			m_PendingValueToSUPlayTo;
    CHXMapPtrToPtr			m_PendingValueToSUSinglePlayTo;
    CHXMapPtrToPtr			m_PendingValueToSULSG;
    CHXMapPtrToPtr			m_PendingValueToSUSingleLSG;

#if !defined(HELIX_CONFIG_NOSTATICS)
    static INT32			zm_nSiteManagerCount;
#else
    static const INT32			zm_nSiteManagerCount;
#endif

    CHXMapStringToOb			m_EventHookMap;
    CHXSimpleList			m_UnnamedEventHookList;


    STDMETHOD(AddSiteByStringHelper)	(THIS_
    					const char*		pString,
					IHXSite*		pSite,
					CHXMapStringToOb&	ByStringMap);

    HXBOOL IsSiteAvailableByStringHelper	(const char*		pString,
					CHXMapStringToOb&	ByStringMap);

    HXBOOL HookupByStringHelper		(const char*		pString,
					CHXMapStringToOb&	ByStringMap,
					IHXSiteUserSupplier*   pSUS,
					HXBOOL			bIsPersistent);

    HXBOOL HookupSingleSiteByStringHelper	(const char*		pString,
					CHXMapStringToOb&	ByStringMap,
					IHXSiteUser*		pSU,
					HXBOOL			bIsPersistent);

    HXBOOL HookupSite2SUS			(IHXSite*		pSite, 
					IHXSiteUserSupplier*   pSUS,
					HXBOOL			bIsPersistent);


    HXBOOL HookupByLSGNameWithString	(IHXSiteUserSupplier*  pSUS, 
					char*			pActualString,
					HXBOOL			bIsPersistent);

    HXBOOL HookupByPlayToFromWithString	(IHXSiteUserSupplier*  pSUS, 
					char*			pActualString,
					HXBOOL			bIsPersistent);


    HXBOOL HookupSingleSiteByLSGNameWithString	(IHXSiteUser*	    pSU,
					char*			    pActualString,
					HXBOOL			    bIsPersistent);

    HXBOOL HookupSingleSiteByPlayToFromWithString	(IHXSiteUser*	    pSU,
					char*			    pActualString,
					HXBOOL			    bIsPersistent);
    

    STDMETHOD(AddEventHookElement)	(CHXSimpleList*		    pList,
					CHXEventHookElement*	    pElement);
    STDMETHOD(RemoveEventHookElement)	(CHXSimpleList*		    pList,
					IHXEventHook*		    pHook,
					UINT16			    uLayer);

    enum EVENT_TYPE
    {
        SITE_EVENT_GENERAL = 0,
        SITE_EVENT_REMOVED,
        SITE_EVENT_ADDED
    };

    enum PTR_TYPE
    {
        SITE_USER_SUPPLIER = 0,
        SITE_USER
    };

    enum HOOK_TYPE
    {
        HOOKUP_BY_LSGNAMEWITHSTRING = 0,
        HOOKUP_BY_PLAYTOFROMWITHSTRING,
        HOOKUP_SINGLESITE_BY_LSGNAMEWITHSTRING,
        HOOKUP_SINGLESITE_BY_PLAYTOFROMWITHSTRING
    };

    HX_RESULT   ProcessSiteEvent        (CHXEventHookElement* pElement, IHXSite* pSite, 
                                        HXxEvent* pEvent, EVENT_TYPE event_type);
    HX_RESULT   HandleSiteEvent         (const char* pRegionName, IHXSite* pSite, 
                                        HXxEvent* pEvent, EVENT_TYPE event_type);
    void        HookupHelper            (CHXMapPtrToPtr* pMap, char* pActualString, HXBOOL bIsPersistent, 
                                        PTR_TYPE ptr_type, HOOK_TYPE hook_type);
    void        RemoveMapStrToObj       (CHXMapStringToOb* pMap);
    void        RemoveMapPtrToPtr       (CHXMapPtrToPtr* pMap);
    void        RemoveList              (CHXSimpleList* pList);

    void CleanupPendingValues();
    ~CHXSiteManager();


public:
    CHXSiteManager(IUnknown* pContext);
    
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /************************************************************************
     *	Method:
     *	    IHXSiteManager::AddSite
     *	Purpose:
     *	  Called to inform the site manager of the existance of a site.
     */
    STDMETHOD(AddSite)		(THIS_
				IHXSite*		pSite);

    /************************************************************************
     *	Method:
     *	    IHXSiteManager::RemoveSite
     *	Purpose:
     *	  Called to inform the site manager that a site is no longer 
     *	  available.
     */
    STDMETHOD(RemoveSite)	(THIS_
				IHXSite*		pSite);

    /*
     * IHXEventHookMgr methods
     */
    /************************************************************************
     *  Method:
     *      IHXEventHookMgr::AddHook
     *  Purpose:
     *    Called to set an event hook that will get events from a site
     *
     */
    STDMETHOD(AddHook)		(THIS_
    				IHXEventHook*		pHook,
				const char*		pRegionName,
				UINT16			uLayer);

    /************************************************************************
     *  Method:
     *      IHXEventHookMgr::RemoveHook
     *  Purpose:
     *    Called to remove an event hook set in AddHook()
     *
     */
    STDMETHOD(RemoveHook)	(THIS_
    				IHXEventHook*		pHook,
				const char*		pRegionName,
				UINT16			uLayer);

    /************************************************************************
     *	Method:
     *	    IHXSiteManager2::GetNumberOfSites
     *	Purpose:
     *	  Called to get the number of sites that the site mananger currently 
     *    knows about.
     */
    STDMETHOD(GetNumberOfSites)		(THIS_  REF(UINT32) nNumSites );

    /************************************************************************
     *	Method:
     *	    IHXSiteManager2::GetSiteAt
     *	Purpose:
     *	  Used to iterate over the sites.
     *	  
     */
    STDMETHOD(GetSiteAt)	(THIS_ UINT32 nIndex, REF(IHXSite*) pSite);
    
    
public:
    /*
     * Methods called internally in PN only code...
     */
    HXBOOL IsSiteAvailableByPlayToFrom(IHXValues*	    pProps,
				    HXBOOL		    bIsPersistent);

    HXBOOL IsSiteAvailableByLSGName   (IHXValues*	    pProps,
				    HXBOOL		    bIsPersistent);

    void UnhookSite		    (IHXSite*		    pSite,
				    HXBOOL		    bIsPersistent);

    void UnhookAll		    ();

    void RemoveSitesByLSGName	    (IHXValues*	    pProps,
				    HXBOOL		    bIsPersistent);

    HXBOOL HookupByLSGName	    (IHXSiteUserSupplier*  pSUS, 
				    IHXValues*		    pProps,
				    HXBOOL		    bIsPersistent);

    HXBOOL HookupByPlayToFrom	    (IHXSiteUserSupplier*  pSUS, 
				    IHXValues*		    pProps,
				    HXBOOL		    bIsPersistent);


    HXBOOL HookupSingleSiteByLSGName  (IHXSiteUser*	    pSU,
				    IHXValues*		    pProps,
				    HXBOOL		    bIsPersistent);

    HXBOOL HookupSingleSiteByPlayToFrom(IHXSiteUser*	    pSU,
				    IHXValues*		    pProps,
				    HXBOOL		    bIsPersistent);

    // mac only
    static void EventOccurred(HXxEvent* pEvent);

    HX_RESULT HandleHookedEvent		(const char*	pRegionName,
					IHXSite*	pSite,
					HXxEvent*	pEvent);

    void HookedSiteAdded		(const char*	pRegionName,
					IHXSite*	pSite);

    void HookedSiteRemoved		(const char*	pRegionName,
					IHXSite*	pSite);

    HXBOOL IsSitePresent              (IHXSite*		    pSite);
    void NeedFocus(HXBOOL bFocus);


#if defined(_MACINTOSH) || defined(_MAC_UNIX) || defined(_WINDOWS)
	static CHXSimpleList	zm_SiteManagerList;
	static HXBOOL 		zm_bWindowRemovedFromList;
//	friend class 		CHXSiteWindowed;
#endif
};

#endif // _SITEMGR_H_

