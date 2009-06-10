/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxupgrd.h,v 1.7 2008/06/06 05:45:15 qluo Exp $
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

#ifndef _HXUPGRD_H
#define _HXUPGRD_H

#include "ihxpckts.h"

typedef _INTERFACE IHXBuffer IHXBuffer;
typedef _INTERFACE IHXValues IHXValues;


/* Enumeration for the upgrade types */
typedef enum _HXUpgradeType
{
    eUT_Required,
    eUT_Recommended,
    eUT_Optional,
    eUT_FileTypeNotSupported
} HXUpgradeType;


/****************************************************************************
 * 
 *  Interface:
 *
 *  	IHXUpgradeCollection
 *
 *  Purpose:
 *
 *	Interface provided by the Context. This interface allows collection 
 *	of upgrade components by the client core and it's delegates 
 *	(i.e. renderer plugins etc.)
 *
 *  IID_IHXUpgradeCollection
 *
 *	{00002500-0901-11d1-8B06-00A024406D59}
 *
 */
 
DEFINE_GUID(IID_IHXUpgradeCollection, 
	    0x00002500, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXUpgradeCollection IID_IHXUpgradeCollection

#undef  INTERFACE
#define INTERFACE   IHXUpgradeCollection

DECLARE_INTERFACE_(IHXUpgradeCollection, IUnknown)
{
    /*
     * IHXUpgradeCollection methods
     */

    /************************************************************************
     *	Method:
     *		IHXUpgradeCollection::Add
     *	Purpose:
     *		Adds the specified upgrade information to the collection
     *
     */
    STDMETHOD_(UINT32, Add)	(THIS_ 
    				HXUpgradeType upgradeType,
				IHXBuffer* pPluginId,
				UINT32 majorVersion,
				UINT32 minorVersion) PURE;

    /************************************************************************
     *	Method:
     *		IHXUpgradeCollection::Remove
     *	Purpose:
     *		Remove the specified item from the collection
     *
     */
    STDMETHOD(Remove)		(THIS_ 
    				UINT32 index) PURE;

    /************************************************************************
     *	Method:
     *		IHXUpgradeCollection::RemoveAll
     *	Purpose:
     *		Remove all items from the collection
     *
     */
    STDMETHOD(RemoveAll)    	(THIS) PURE;

    /************************************************************************
     *	Method:
     *		IHXUpgradeCollection::GetCount
     *	Purpose:
     *		get the count of the collection
     *
     */
    STDMETHOD_(UINT32, GetCount)(THIS) PURE;

    /************************************************************************
     *	Method:
     *		IHXUpgradeCollection::GetAt
     *	Purpose:
     *		get the specified items upgrade information
     *
     */
    STDMETHOD(GetAt)		(THIS_ 
    				UINT32 index,
				REF(HXUpgradeType) upgradeType,
				IHXBuffer* pPluginId,
				REF(UINT32) majorVersion, 
				REF(UINT32) minorVersion) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *  	IHXUpgradeHandler
 *
 *  Purpose:
 *
 *	Interface provided by the top-level client application.  This
 *	interface allows the client core to request an upgrade.
 *
 *  IID_IHXUpgradeHandler:
 *
 *	{00002501-0901-11d1-8B06-00A024406D59}
 *
 */
 
DEFINE_GUID(IID_IHXUpgradeHandler, 
                        0x00002501, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXUpgradeHandler

DECLARE_INTERFACE_(IHXUpgradeHandler, IUnknown)
{
    /*
     * IHXUpgradeHandler methods
     */

    /************************************************************************
     *	Method:	    IHXUpgradeHandler::RequestUpgrade
     *	Purpose:    Send a request for an upgrade of particular set of components.
     *		    If new components are available ask user and start upgrade.
     *  Arguments:
     *	    pUpgradeCollection - interface that determines what components
     *				 to upgrade.
     *      bBlocking          - flag of not returning from this call until
     *				 upgrade is done.
     *  Return Value:
     *	    HXR_OK             - upgrade request was sent;
     *				 if bBlocking == TRUE new components received
     *				 and sucesfully installed.
     *	    HXR_FAILED	       - upgrade failed.
     *	    HXR_NO_DATA        - user cancelled the upgrade or
     *				 no new data available at this time.
     */
    STDMETHOD(RequestUpgrade) (THIS_ IHXUpgradeCollection* pComponents,
				     HXBOOL bBlocking) PURE;

    /************************************************************************
     *	Method:
     *		IHXUpgradeHandler::HasComponents
     *	Purpose:
     *		Check if required components are present on the system.
     *  Returns:
     *		HXR_OK - components are here, no upgrade required;
     *			 all components are removed from pComponents.
     *          HXR_FAIL - some components are missing;
     *                   pComponents contains only those components 
     *			 that need upgrade.
     *
     */			      
    STDMETHOD(HasComponents)  (THIS_ IHXUpgradeCollection* pComponents) PURE;
};


// $Private:
/****************************************************************************
 * 
 *  Interface:
 *
 *  	IHXSystemRequired
 *
 *  Purpose:
 *
 *	Interface provided by the client core. It can be replaced by the top 
 *	level client. 
 *
 *  IID_IHXSystemRequired:
 *
 *	{00002502-0901-11d1-8B06-00A024406D59}
 *
 */
 
DEFINE_GUID(IID_IHXSystemRequired, 
		0x00002502, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSystemRequired

DECLARE_INTERFACE_(IHXSystemRequired, IUnknown)
{
    /*
     * IHXSystemRequired methods
     */

    /************************************************************************
     *	Method:
     *		IHXSystemRequired::HasFeatures
     *	Purpose:
     *		Check if required features are present on the system.
     *  Returns:
     *		HXR_OK -    features are here, no upgrade required;
     *			    all features are removed from pFeatures.
     *		HXR_FAIL -  some features are missing;
     *			    pFeatures contains only those features 
     *			    that need upgrade.
     *
     */			      
    STDMETHOD(HasFeatures)  (THIS_ 
			    IHXUpgradeCollection* pFeatures) PURE;
};
// $EndPrivate.

/****************************************************************************
 * 
 *  Interface:
 *
 *  	IHXUpgradeCollection2
 *
 *  Purpose:
 *
 *	Interface provided by the Context. Allows to add additional information
 *	to be passed to upgrade server for a given component
 *
 *  IID_IHXUpgradeCollection2
 *
 *	{00002503-0901-11d1-8B06-00A024406D59}
 *
 */
 
DEFINE_GUID(IID_IHXUpgradeCollection2, 
	    0x00002503, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXUpgradeCollection2

DECLARE_INTERFACE_(IHXUpgradeCollection2, IUnknown)
{
    /*
     * IHXUpgradeCollection2 methods
     */

    /************************************************************************
     *	Method:
     *		IHXUpgradeCollection::AddURLParseElement
     *	Purpose:
     *		Adds name-value pair for RUP URL parsing:
     *		URL-encoded values substitute names.
     *
     */
    STDMETHOD(AddURLParseElement) (THIS_ const char* pName,
					 const char* Value) PURE;

    /************************************************************************
     *	Method:
     *		IHXUpgradeCollection::GetURLParseElements
     *	Purpose:
     *		Gets name-value pair for RUP URL parsing.
     *
     */
    STDMETHOD(GetURLParseElements) (THIS_ REF(IHXValues*) pURLParseElements) PURE;
};

#endif /* _HXUPGRD_H */
 
