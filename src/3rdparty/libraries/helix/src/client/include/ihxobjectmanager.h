/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ihxobjectmanager.h,v 1.4 2007/07/06 21:58:18 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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

#ifndef _IHXOBJMAN_H
#define _IHXOBJMAN_H

#include "hxcom.h"
#include "hxcomptr.h"

/*!
    @singletype IHXObjectManager
    @abstract The ObjectManager provides scope and alias support for the object-broker
    @discussion 
    Object Managers provide a caching mechanism.  While requests for 
objects from the object factory always result in the creation of a new 
object, requests from an object manager might retrive an already existing 
object.

The system uses AddObjectToInstanceCache() to indicate that an object 
should be cached in the object manager.  AddObjectToInstanceCache() 
doesn't create the object immediately.  The object's creation is defered 
untill the first time the clsid is requested of the object manager through 
the IHXCommonClassFactory interface.

When a clsid is requested of the object manager, it returns an instance 
from its cache, if possible.  If it has no entry in its cache for the 
clsid, the object manager defers to its parent.

Object managers can be nested, so it's possible to create a fairly complex 
tree of object managers.  This is how the application can scope the 
various components and services.  In general, it seems unlikely that there 
will be more than two or three levels of scope.  The general case might be 
two object managers set up like this:

An application-level object manager caches system services like 
preferences or the playback manager.

Each window is mantained in a frame-level object manager that caches 
per-window services, i.e. the window, player, and playstate manager.
   
The application may create an additional object manager to cache the 
collection or objects necessary for auto-update.  This allows the 
application to release all AU compoents by Release()ing the AU 
object-manager.

The object manager also supports the idea of aliasing on clsid to another.  
If an alias from clsid1 to clsid2 is set up, then whenever clsid1 is 
requested through the IHXCommonClassFactory interface, an object of type 
clsid2 is returned.

The object manager allows the application to see if a cached object has 
been instantiated.  IsCachedObjectLoaded() traverses the this object 
manager and all it's parents to see if an instance of clsid has been 
cached and loaded.  If the clsid has not been cached, IsCachedObjectLoaded() 
returns an error.  Otherwise, the result of the query is returned in 
pbResult.
 
*/


// $Private:


// {919DBA72-AED0-11d3-B64F-00105A121185}
DEFINE_GUID(IID_IHXObjectManager, 0x919dba72, 0xaed0, 0x11d3, 0xb6, 0x4f, 0x0, 0x10, 0x5a, 0x12, 0x11, 0x85);


DECLARE_INTERFACE_( IHXObjectManager, IUnknown )
{
    /*!
	@function	SetDefaultHoldTime
	@abstract	Sets the default time to keep unreferenced objects' instances in the cache before 
			purging them.
	@param		holdTime [in] The default amount of time to hold objects in milliseconds.
	@result		Returns HXR_OK on success.  An appropriate error code otherwise.
    */
    STDMETHOD(SetDefaultHoldTime) (THIS_ UINT32 holdTime) PURE;

    /*!
	@function	SetInfiniteDefaultHoldTime
	@abstract	Sets the default time to keep unreferenced objects' instances in the cache before 
			purging them to infinite.
	@result		Returns HXR_OK on success.  An appropriate error code otherwise.
    */
    STDMETHOD(SetInfiniteDefaultHoldTime) (THIS) PURE;

    /*!
	@function	SetCachedObjectHoldTime
	@abstract	Sets the time to keep an unreferenced object's instance in the cache before
			purging it.
	@param		holdTime [in] The amount of time to hold the object in milliseconds.
	@param		clsid [in] The CLSID of the object.
	@result		Returns HXR_OK on success.  HXR_UNEXPECTED if the object is not cached & an appropraite error code otherwise.
    */
    STDMETHOD(SetCachedObjectHoldTime) (THIS_ REFCLSID clsid, UINT32 holdTime) PURE;

    /*!
	@function	SetInfiniteCachedObjectHoldTime
	@abstract	Sets the time to keep an unreferenced object's instance in the cache before
			purging it to infinite.
	@param		clsid [in] The CLSID of the object.
	@result		Returns HXR_OK on success.  HXR_UNEXPECTED if the object is not cached & an appropraite error code otherwise.
    */
    STDMETHOD(SetInfiniteCachedObjectHoldTime) (THIS_ REFCLSID clsid) PURE;

    /*!
	@function 	AddObjectToInstanceCache
	@abstract 	Adds an empty slot for clsid to this cache.
	@param 		clsid [in] The CLSID of the object to cache.
	@result 	Returns HXR_OK on success.  An appropriate error code otherwise.
     */
    STDMETHOD(AddObjectToInstanceCache) (THIS_ REFCLSID clsid) PURE;

    /*!
	@function 	AddAliasToInstanceCache
	@abstract 	Adds a remap from the CLSID alias to the CLSID actualCLSID.
	@param 		alias [in] The CLSID to remap
	@param 		actualCLSID [in] The CLSID of the object that is returned.
	@result 	Returns HXR_OK on success.  HXR_UNEXPECTED if the CLSID alias is already used.
     */
    STDMETHOD(AddAliasToInstanceCache) (THIS_ REFCLSID alias, REFCLSID actualCLSID) PURE;

    /*!
	@function 	RemoveAliasFromInstanceCache
	@abstract 	Removes a remap from the CLSID alias to the CLSID actualCLSID.
	@param 		alias [in] The CLSID to no longer remap
	@result 	Returns HXR_OK on success.
     */
    STDMETHOD(RemoveAliasFromInstanceCache) (THIS_ REFCLSID alias) PURE;

    /*!
	@function 	RemoveObjectFromInstanceCache
	@abstract 	Removes the entry for the specified object from this cache
	@param 		clsid [in] The CLSID of the object to remove from the cache
	@result 	Returns HXR_OK on success.  HXR_FAIL otherwise.
	
     */
    STDMETHOD(RemoveObjectFromInstanceCache) (THIS_ REFCLSID clsid) PURE;

    /*!
	@function 	IsCachedObjectLoaded
	@abstract 	Checks to see if a cached object has been instantiated
	@discussion     The object manager allows the application to see if a cached object has 
			been instantiated.  IsCachedObjectLoaded() traverses this object 
			manager and all it's parents to see if an instance of clsid has been 
			cached and loaded.  If the clsid has not been cached, IsCachedObjectLoaded() 
			returns an error.  Otherwise, the result of the query is returned in 
			pbResult.
	@param		clsid [in] The CLSID of the object to check.
	@param 		pbResult [out] Set to FALSE if calling CreateInstance() with this clsid would result in the allocation of a new object.
	@result 	HXR_UNEXPECTED if the object is not cached, HXR_OK otherwise.
     */
    STDMETHOD(IsCachedObjectLoaded) (THIS_ REFCLSID clsid, HXBOOL* pbResult ) PURE;

    /*!
	@function 	FlushCaches
	@abstract 	Clears the instance, and observer caches
	@result 	Always return HXR_OK;
     */
    STDMETHOD(FlushCaches) (THIS) PURE;

    /*!
	@function 	CreateParentCachedInstance
	@abstract 	Create the corresponding clsid within the parent object manager
			hierarchy of this object.
	@param		clsid [in] The CLSID of the object to create.
	@param 		ppIUnknown [out] Returns the created object.
	@result 	Succeeds if object was created and cached in a parent context;
     */
    STDMETHOD(CreateParentCachedInstance)	(THIS_ REFCLSID clsid, IUnknown** ppIUnknown) PURE;

    /*!
	@function 	GetAliasedCLSID
	@abstract 	Return the alias (if any) for the given CLSID
	@param 		clsid [in] The aliased CLSID
	@param 		pAliasCLSID [out] The CLSID that clsid is aliased as.
	@result 	Returns HXR_OK on success.  HXR_FAIL otherwise.
	
     */
    STDMETHOD(GetAliasedCLSID) (THIS_ REFCLSID clsid, CLSID* pAliasCLSID) PURE;
};

DEFINE_SMART_PTR(IHXObjectManager);

// $EndPrivate.



#endif // _IHXOBJMAN_H
