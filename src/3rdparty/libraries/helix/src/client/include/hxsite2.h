/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsite2.h,v 1.8 2007/01/26 01:44:19 ping Exp $
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

#ifndef _HXSITE2_H_
#define _HXSITE2_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE  IHXSite2			    IHXSite2;
typedef _INTERFACE  IHXSite3			    IHXSite3;
typedef _INTERFACE  IHXSiteTreeNavigation           IHXSiteTreeNavigation;
typedef _INTERFACE  IHXVideoSurface		    IHXVideoSurface;
typedef _INTERFACE  IHXPassiveSiteWatcher	    IHXPassiveSiteWatcher;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSite2
 *
 *  Purpose:
 *
 *	Interface for IHXSite2 objects.
 *
 *  IID_IHXSite:
 *
 *	{0x00000D0A-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXSite2, 0x00000D0A, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSite2

DECLARE_INTERFACE_(IHXSite2, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXSite2 method usually called by the "context" 
     * when window attributes (like the window handle) have changed.
     */
    STDMETHOD(UpdateSiteWindow) (THIS_
				HXxWindow* /*IN*/ pWindow) PURE;

    /*
     * IHXSite2 method usually called by the "context" to
     * to hide/show a site.
     */
    STDMETHOD(ShowSite)         (THIS_
                                 HXBOOL    bShow) PURE;
                                 
    STDMETHOD_(HXBOOL, IsSiteVisible)         (THIS) PURE;

    /*
     * IHXSite2 method usually called by the "context" to
     * set the site's Z-order
     */
    STDMETHOD(SetZOrder)	(THIS_
				INT32 lZOrder
				) PURE;

    /*
     * IHXSite2 method called to get the site's Z-order
     */
    STDMETHOD(GetZOrder)	(THIS_
				REF(INT32) lZOrder
				) PURE;

    /*
     * IHXSite2 method called to set the site at the top
     * of the Z-order
     */
    STDMETHOD(MoveSiteToTop)	(THIS) PURE;

    /*
     * IHXSite2 method called to get the site's video surface
     */
    STDMETHOD(GetVideoSurface)	(THIS_ 
				REF(IHXVideoSurface*) pSurface
				) PURE;

    /*
     * IHXSite2 method called to get the number of child sites.
     */
    STDMETHOD_(UINT32,GetNumberOfChildSites) (THIS) PURE;

    /*
     * IHXSite2 method to add a watcher that does not affect the site
     */
    STDMETHOD(AddPassiveSiteWatcher)	(THIS_
    					IHXPassiveSiteWatcher* pWatcher
					) PURE;

    /*
     * IHXSite2 method to remove a watcher that does not affect the site
     */
    STDMETHOD(RemovePassiveSiteWatcher) (THIS_
    					IHXPassiveSiteWatcher* pWatcher
					) PURE;

    /*
     * IHXSite2 method used to do cursor management
     */
    STDMETHOD(SetCursor) 		(THIS_
    					HXxCursor ulCursor,
					REF(HXxCursor) ulOldCursor
					) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSite3 (Created 1/24/2007)
 *
 *  Purpose:
 *
 *	Interface for IHXSite3 objects.
 *
 *  IID_IHXSite:
 *
 *	{0x00000D0A-0901-11d1-8B06-00A024406D69}
 *
 */
DEFINE_GUID(IID_IHXSite3, 0x00000D0A, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x69);


#undef  INTERFACE
#define INTERFACE   IHXSite3

DECLARE_INTERFACE_(IHXSite3, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    STDMETHOD(CreateChild)	(THIS_
                                HXBOOL          /*IN*/ bWindowless,
				REF(IHXSite*)	/*OUT*/ pChildSite) PURE;

    STDMETHOD(SetClipRect)      (THIS_ const REF(HXxRect) rect) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSiteTreeNavigation
 *
 *  Purpose:
 *
 *	Interface for IHXSiteTreeNavigation objects.
 *
 *  IID_IHXSiteTreeNavigation:
 *
 *	{b52abc41-a919-11d8-b8a3-0003939ba95e}
 *
 */

DEFINE_GUID(IID_IHXSiteTreeNavigation, 0xb52abc41, 0xa919, 0x11d8, 0xb8, 0xa3,
                            0x0, 0x03, 0x93, 0x9b, 0xa9, 0x5e);

#undef  INTERFACE
#define INTERFACE   IHXSiteTreeNavigation

DECLARE_INTERFACE_(IHXSiteTreeNavigation, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;
    
    /*
     * IHXSiteTreeNavigation methods
     */

    STDMETHOD(GetParentSite) (THIS_
                             REF(IHXSite*) pParentSite
                             ) PURE;

    STDMETHOD_(UINT32, GetNumberOfChildSites) (THIS) PURE;

    STDMETHOD(GetNthChildSite) (THIS_
                                ULONG32 ulIndex,
                                REF(IHXSite*) pSite
                                ) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPassiveSiteWatcher
 *
 *  Purpose:
 *
 *	Interface for IHXPassiveSiteWatcher objects.
 *
 *  IID_IHXPassiveSiteWatcher:
 *
 *	{0x00000D0F-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPassiveSiteWatcher, 0x00000D0F, 0x901, 0x11d1, 0x8b, 0x6, 
			0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPassiveSiteWatcher

DECLARE_INTERFACE_(IHXPassiveSiteWatcher, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXPassiveSiteWatcher method used to notify
     * about position updates
     */
    STDMETHOD(PositionChanged) (THIS_
				HXxPoint* /*IN*/ pPoint) PURE;

    /*
     * IHXPassiveSiteWatcher method used to notify
     * about size updates
     */
    STDMETHOD(SizeChanged) 	(THIS_
				HXxSize* /*IN*/ pSize) PURE;

};


/***********************************************************************/
/********  PRIVATE INTERFACE UNTIL STABALIZED -- WILL CHANGE ***********/
/***********************************************************************/
/*
 *
 *  Interface:  IHXSiteControl
 *
 *  Purpose:
 *           Helps manage multiple sites and their children
 *
 *  Interface for obtaining IHXSiteControl
 *
 *  {DD25CA2E-73A5-4811-996F-7E6726E7668F}
 *
 */
DEFINE_GUID(IID_IHXSiteControl, 0xdd25ca2e, 0x73a5, 0x4811, 0x99,
            0x6f, 0x7e, 0x67, 0x26, 0xe7, 0x66, 0x8f);
#undef  INTERFACE
#define INTERFACE   IHXSiteControl
DECLARE_INTERFACE_(IHXSiteControl, IUnknown)
{
    /* redraws this site and all children */
    STDMETHOD(ForceRedrawAll) (THIS_ ) PURE;
};
/***********************************************************************/
/********  PRIVATE INTERFACE UNTIL STABALIZED -- WILL CHANGE ***********/
/***********************************************************************/

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXSite2)
DEFINE_SMART_PTR(IHXSite3)
DEFINE_SMART_PTR(IHXSiteTreeNavigation)
DEFINE_SMART_PTR(IHXPassiveSiteWatcher)
DEFINE_SMART_PTR(IHXSiteControl)

#endif //_HXSITE2_H_
