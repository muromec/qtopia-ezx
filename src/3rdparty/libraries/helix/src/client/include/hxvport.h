/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxvport.h,v 1.3 2007/07/06 21:58:18 jfinnecy Exp $
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

#ifndef _HXVPORT_H_
#define _HXVPORT_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE	IHXValues			IHXValues;
typedef _INTERFACE	IHXViewPortManager		IHXViewPortManager;
typedef _INTERFACE	IHXViewPort			IHXViewPort;
typedef _INTERFACE	IHXViewPortSink		IHXViewPortSink;
typedef _INTERFACE	IHXSiteUser			IHXSiteUser;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXViewPortManager
 *
 *  Purpose:
 *
 *	Interface to manage IHXViewPort
 *
 *  IID_IHXViewPortManager:
 *
 *	{00004000-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXViewPortManager, 0x00004000, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			     0xa0, 0x24, 0x40, 0x6d, 0x59);

DECLARE_INTERFACE_(IHXViewPortManager, IUnknown)
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
     * IHXViewPortManager methods
     */

    /************************************************************************
     *	Method:
     *	    IHXViewPortManager::OpenViewPort
     *	Purpose:
     *	    create viewport
     */
    STDMETHOD(OpenViewPort)	(THIS_
				 IHXValues*	pValues,
				 IHXSiteUser*	pSiteUser) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPortManager::GetViewPort
     *	Purpose:
     *	    get viewport
     */
    STDMETHOD(GetViewPort)	(THIS_
				 const char* pszViewPort,
				 REF(IHXViewPort*) pViewPort) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPortManager::CloseViewPort
     *	Purpose:
     *	    remove viewport
     */
    STDMETHOD(CloseViewPort)	(THIS_
				 const char* pszViewPort) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPortManager::AddViewPortSink
     *	Purpose:
     *	    add viewport sinker
     */
    STDMETHOD(AddViewPortSink)	(THIS_
				 IHXViewPortSink*  pViewPortSink) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPortManager::RemoveViewPortSink
     *	Purpose:
     *	    remove viewport sinker
     */
    STDMETHOD(RemoveViewPortSink)   (THIS_
				     IHXViewPortSink*  pViewPortSink) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXViewPort
 *
 *  Purpose:
 *
 *	Interface to IHXViewPort
 *
 *  IID_IHXViewPort
 *
 *	{00004001-0901-11d1-8B06-00A024406D59}
 */
DEFINE_GUID(IID_IHXViewPort, 0x00004001, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
				    0xa0, 0x24, 0x40, 0x6d, 0x59);

DECLARE_INTERFACE_(IHXViewPort, IUnknown)
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
     * IHXViewPort methods
     */

    /************************************************************************
     *	Method:
     *	    IHXViewPort::GetName
     *	Purpose:
     *	    get name of the viewport
     */
    STDMETHOD_(const char*, GetName)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPort::GetProperties
     *	Purpose:
     *	    get properties of the viewport
     */
    STDMETHOD(GetProperties)	(THIS_
				 REF(IHXValues*)   pValues) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPort::Show
     *	Purpose:
     *	    show viewport
     */
    STDMETHOD(Show)		(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPort::Hide
     *	Purpose:
     *	    hide viewport
     */
    STDMETHOD(Hide)		(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPort::SetFocus
     *	Purpose:
     *	    set focus on viewport
     */
    STDMETHOD(SetFocus)		(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPort::SetZOrder
     *	Purpose:
     *	    set Z order on viewport
     */
    STDMETHOD(SetZOrder)	(THIS_
				 UINT32	ulZOrder) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXViewPortSink
 *
 *  Purpose:
 *
 *	Interface sinker to IHXViewPort
 *
 *  IID_IHXViewPortSink
 *
 *	{00004002-0901-11d1-8B06-00A024406D59}
 */
DEFINE_GUID(IID_IHXViewPortSink, 0x00004002, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
				    0xa0, 0x24, 0x40, 0x6d, 0x59);

DECLARE_INTERFACE_(IHXViewPortSink, IUnknown)
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
     * IHXViewPortSink methods
     */

    /************************************************************************
     *	Method:
     *	    IHXViewPortSink::ViewPortOpened
     *	Purpose:
     *	    notification of the addition of viewport
     */
    STDMETHOD(ViewPortOpened)	(THIS_
				 const char*	pszViewPort) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPortSink::ViewPortClosed
     *	Purpose:
     *	    notification of the removal of viewport
     */
    STDMETHOD(ViewPortClosed)	(THIS_
				 const char*	pszViewPort) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPortSink::ViewPortShown
     *	Purpose:
     *	    notification of the shown of viewport
     */
    STDMETHOD(ViewPortShown)	(THIS_
				 const char*	pszViewPort) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPortSink::ViewPortHidden
     *	Purpose:
     *	    notification of the hide of viewport
     */
    STDMETHOD(ViewPortHidden)	(THIS_
				 const char*	pszViewPort) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPortSink::ViewPortFocusSet
     *	Purpose:
     *	    notification of the active focus of viewport
     */
    STDMETHOD(ViewPortFocusSet)	(THIS_
				 const char*	pszViewPort) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPortSink::ViewPortZOrderSet
     *	Purpose:
     *	    notification of the Z order of viewport
     */
    STDMETHOD(ViewPortZOrderSet)(THIS_
				 const char*	pszViewPort,
				 UINT32		ulZOrder) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXViewPortSupplier
 *
 *  Purpose:
 *
 *	Interface IHXViewPortSupplier
 *
 *  IID_IHXViewPortSupplier
 *
 *	{00004003-0901-11d1-8B06-00A024406D59}
 */
DEFINE_GUID(IID_IHXViewPortSupplier, 0x00004003, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
				    0xa0, 0x24, 0x40, 0x6d, 0x59);

DECLARE_INTERFACE_(IHXViewPortSupplier, IUnknown)
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
     * IHXViewPortSupplier methods
     */

    /************************************************************************
     *	Method:
     *	    IHXViewPortSupplier::OnViewPortOpen
     *	Purpose:
     *	    notification of the addition of viewport
     */
    STDMETHOD(OnViewPortOpen)	(THIS_
				 IHXValues* pValues,
				 IHXSiteUser* pSiteUser) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPortSupplier::OnViewPortClose
     *	Purpose:
     *	    notification of the removal of viewport
     */
    STDMETHOD(OnViewPortClose)	(THIS_
				 const char*	pszViewPort) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPortSupplier::OnViewPortShow
     *	Purpose:
     *	    notification of the shown of viewport
     */
    STDMETHOD(OnViewPortShow)	(THIS_
				 const char*	pszViewPort) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPortSupplier::OnViewPortHide
     *	Purpose:
     *	    notification of the hide of viewport
     */
    STDMETHOD(OnViewPortHide)	(THIS_
				 const char*	pszViewPort) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPortSupplier::OnViewPortFocus
     *	Purpose:
     *	    notification of the active focus of viewport
     */
    STDMETHOD(OnViewPortFocus)	(THIS_
				 const char*	pszViewPort) PURE;

    /************************************************************************
     *	Method:
     *	    IHXViewPortSupplier::OnViewPortZOrder
     *	Purpose:
     *	    notification of the Z order of viewport
     */
    STDMETHOD(OnViewPortZOrder)	(THIS_
				 const char*	pszViewPort,
				 UINT32		ulZOrder) PURE;
};
#endif /* _HXVPORT_H_ */
