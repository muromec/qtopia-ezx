/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: upgrdcol.h,v 1.4 2006/02/09 01:09:53 ping Exp $
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

#ifndef _UPGRDCOL_H
#define _UPGRDCOL_H

#include "hxupgrd.h"
#include "carray.h"


/****************************************************************************
 * 
 *  Interface:
 *
 *  	IHXUpgradeCollection
 *
 *  Purpose:
 *
 *	Interface provided by the client engine. This
 *	interface allows collection of upgrade components by the client
 *	core and it's delegates (i.e. renderer plugins etc.)
 *
 *  IID_IHXUpgradeCollection
 *
 *	{BD387A00-32CB-11d1-9907-444553540000}
 *
 */
 

class HXUpgradeCollection : public IHXUpgradeCollection,
			    public IHXUpgradeCollection2
{
public:

    HXUpgradeCollection(IUnknown* pContext);
    
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IRMUpgradeCollection methods
     */

    /************************************************************************
     *	Method:
     *		IHXUpgradeCollection::Add
     *	Purpose:
     *		Adds the specified upgrade information to the collection
     *
     */
    STDMETHOD_(UINT32, Add)(THIS_ 
    				HXUpgradeType upgradeType, IHXBuffer* pPluginId,
				UINT32 majorVersion, UINT32 minorVersion);

    /************************************************************************
     *	Method:
     *		IHXUpgradeCollection::Remove
     *	Purpose:
     *		remove the specified item from the collection
     *
     */
    STDMETHOD(Remove)(THIS_ 
    			UINT32 index);

    /************************************************************************
     *	Method:
     *		IHXUpgradeCollection::RemoveAll
     *	Purpose:
     *		remove all items from the collection
     *
     */
    STDMETHOD(RemoveAll)(THIS);

    /************************************************************************
     *	Method:
     *		IHXUpgradeCollection::GetCount
     *	Purpose:
     *		get the count of the collection
     *
     */
    STDMETHOD_(UINT32, GetCount)(THIS);
    
    /************************************************************************
     *	Method:
     *		IHXUpgradeCollection::GetAt
     *	Purpose:
     *		get the specified items upgrade information
     *
     */
    STDMETHOD(GetAt)(THIS_ 
    			UINT32 index, REF(HXUpgradeType) upgradeType,
			IHXBuffer* pPluginId, REF(UINT32) majorVersion, 
			REF(UINT32) minorVersion);


    /*
     * IRMUpgradeCollection2 methods
     */

    STDMETHOD(AddURLParseElement) (THIS_ const char* pName, const char* Value);
    STDMETHOD(GetURLParseElements) (THIS_ REF(IHXValues*) pURLParseElements);

protected:

    ~HXUpgradeCollection(void);

    LONG32 m_lRefCount;		// reference count
    IUnknown* m_pContext;
    CHXPtrArray* m_pComponents;	// the components to upgrade
    IHXValues*  m_pURLParseElements;	// Name-value pairs for RUP URL parsing.
};
#endif		// _UPGRDCOL_H


