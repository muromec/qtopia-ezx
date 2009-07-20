/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: viewport.h,v 1.3 2004/07/09 18:42:32 hubbe Exp $
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

#ifndef _VIEWPORT_H_
#define _VIEWPORT_H_

#include "hxvport.h"

class HXViewPortManager;

class HXViewPort : public IHXViewPort
{
private:
    LONG32		m_lRefCount;
    
    const char*		m_pszName;
    IHXValues*		m_pValues;
    HXViewPortManager*	m_pParent;

    ~HXViewPort();


public:
    HXViewPort(HXViewPortManager* pViewPortManager,
		IHXValues*	    pValues,
		const char*	    pszName);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXViewPort methods
     */

    /************************************************************************
     *	Method:
     *	    IHXViewPort::GetName
     *	Purpose:
     *	    get name of the viewport
     */
    STDMETHOD_(const char*, GetName)	(THIS);

    /************************************************************************
     *	Method:
     *	    IHXViewPort::GetProperties
     *	Purpose:
     *	    get properties of the viewport
     */
    STDMETHOD(GetProperties)	(THIS_
				 REF(IHXValues*)   pValues);

    /************************************************************************
     *	Method:
     *	    IHXViewPort::Show
     *	Purpose:
     *	    show viewport
     */
    STDMETHOD(Show)		(THIS);

    /************************************************************************
     *	Method:
     *	    IHXViewPort::Hide
     *	Purpose:
     *	    hide viewport
     */
    STDMETHOD(Hide)		(THIS);

    /************************************************************************
     *	Method:
     *	    IHXViewPort::SetFocus
     *	Purpose:
     *	    set focus on viewport
     */
    STDMETHOD(SetFocus)		(THIS);

    /************************************************************************
     *	Method:
     *	    IHXViewPort::SetZOrder
     *	Purpose:
     *	    set Z order on viewport
     */
    STDMETHOD(SetZOrder)	(THIS_
				 UINT32	ulZOrder);
};

class HXViewPortManager : public IHXViewPortManager
{
private:
    LONG32		    m_lRefCount;
    
    HXPlayer*		    m_pPlayer;
    CHXMapStringToOb*	    m_pViewPortMap;
    CHXSimpleList*	    m_pViewPortSinkList;
    IHXViewPortSupplier*   m_pViewPortSupplier;

    friend class	    HXViewPort;

    ~HXViewPortManager();


protected:
    HX_RESULT	OnViewPortShow(const char* pszViewPortName);
    HX_RESULT	OnViewPortHide(const char* pszViewPortName);
    HX_RESULT	OnViewPortFocus(const char* pszViewPortName);
    HX_RESULT	OnViewPortZOrder(const char* pszViewPortName, UINT32 ulZOrder);
    
public:
    HXViewPortManager(HXPlayer* pPlayer);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

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
				 IHXSiteUser*  pSiteUser);

    /************************************************************************
     *	Method:
     *	    IHXViewPortManager::GetViewPort
     *	Purpose:
     *	    get viewport
     */
    STDMETHOD(GetViewPort)	(THIS_
				 const char* pszViewPort,
				 REF(IHXViewPort*) pViewPort);

    /************************************************************************
     *	Method:
     *	    IHXViewPortManager::CloseViewPort
     *	Purpose:
     *	    remove viewport
     */
    STDMETHOD(CloseViewPort)	(THIS_
				 const char* pszViewPort);

    /************************************************************************
     *	Method:
     *	    IHXViewPortManager::AddViewPortSink
     *	Purpose:
     *	    add viewport sinker
     */
    STDMETHOD(AddViewPortSink)	(THIS_
				 IHXViewPortSink*  pViewPortSink);

    /************************************************************************
     *	Method:
     *	    IHXViewPortManager::RemoveViewPortSink
     *	Purpose:
     *	    remove viewport sinker
     */
    STDMETHOD(RemoveViewPortSink)   (THIS_
				     IHXViewPortSink*  pViewPortSink);

    void Close();
};

#endif /* _VIEWPORT_H_ */

