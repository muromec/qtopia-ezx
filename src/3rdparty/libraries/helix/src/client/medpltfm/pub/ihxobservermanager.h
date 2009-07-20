/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ihxobservermanager.h,v 1.1 2006/03/22 17:52:00 stanb Exp $
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


/*!
    @singletype IHXObserverManager
    @abstract Manager interface to handle the late binding of observers
    @discussion 
	Calling AddObserver() doesn't necessarily set up an observer relationship 
	immediately.  AddObserver() puts the pIObserver into a list of things to 
	be added as observers when the object represented by clsid is actually 
	loaded.  Of course, if the object is already loaded, the observer 
	relationship is set up immediately.
	
	The point of this manager is to allow components to set up complex 
	observer relationships during initialization without requiring all the 
	susbsytems involed to be loaded.  The playback engine is a prime example 
	of this.  The actor that manages playstate will want to get notifications 
	from the player object.  The player object, however, doesn't need to be 
	loaded untill playback has actually been initiated.  The actor can use 
	this manager to set up late-binding to the playback engine, and then not 
	worry about whether the playback engine is loaded or unloaded.
    
 */

#ifndef _IHXOBSERVERMGR_H
#define _IHXOBSERVERMGR_H

#include "hxcom.h"
#include "hxcomptr.h"

_INTERFACE IHXObserver;


// $Private;

// {5DE44BA4-D435-11d3-B654-00105A121185}
DEFINE_GUID(IID_IHXObserverManager, 
    0x5de44ba4, 0xd435, 0x11d3, 0xb6, 0x54, 0x0, 0x10, 0x5a, 0x12, 0x11, 0x85);

#undef  INTERFACE
#define INTERFACE   IHXObserverManager

/*!
    @interface IHXObserverManager
 */
DECLARE_INTERFACE_(IHXObserverManager, IUnknown)
{
    /*!
	@function 	AddObserver
	@abstract 	Adds the specified IHXObserver to the list of observers for the specified clsid 
	@param 		clsid [in] The CLSID of the component to be observered
	@param 		pIObserver [in] The object doing the observing.  It must implement the IHXObserver interface.
	@result 	Returns HXR_OK on success.  An appropriate error code otherwise.
     */
    STDMETHOD(AddObserver) (THIS_ REFCLSID clsid, IHXObserver* pIObserver) PURE;

    /*!
	@function 	RemoveObserver
	@abstract 	Removes the specified IHXObserver from the list of observers for the specified clsid
	@param 		clsid [in] The CLSID of the component being observered
	@param 		pIObserver [in] The object that has been doing the observing.  It must implement the IHXObserver interface.
	@result 	Returns HXR_OK on success.  An appropriate error code otherwise.
     */
    STDMETHOD(RemoveObserver) (THIS_ REFCLSID clsid, IHXObserver* pIObserver) PURE;
};

DEFINE_SMART_PTR(IHXObserverManager);

// $EndPrivate.



#endif //_IHXOBSERVERMGR_H
