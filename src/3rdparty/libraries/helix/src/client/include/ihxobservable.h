/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: ihxobservable.h,v 1.1 2006/03/22 17:51:59 stanb Exp $
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
    @singletype IHXObservable
    @abstract Interface to be implemented by services that wish to be observable using 
    the IHXObserverManager architecture.
    @discussion The ObserverManager calls these functions once per registered observer
    when this object is loaded or unloaded.
    
    Aside from setting up this relationship, the ObserverManager makes no assumptions
    or requirements about what notifications might be sent or any other communication
    between the two objects.  Once AddObserver() has been called, it is up to the two
    objects to discover appropriate notification interfaces.  It is also the responsibility
    of the two objects to disconnect and Release() these additional interfaces.
 */


#ifndef _IHXOBSERVABLE_H
#define _IHXOBSERVABLE_H

#include "hxcom.h"
#include "hxcomptr.h"

_INTERFACE IHXObserver;



// $Private;

// {5DE44BA3-D435-11d3-B654-00105A121185}
DEFINE_GUID(IID_IHXObservable, 
    0x5de44ba3, 0xd435, 0x11d3, 0xb6, 0x54, 0x0, 0x10, 0x5a, 0x12, 0x11, 0x85);

#undef  INTERFACE
#define INTERFACE   IHXObservable

/*!
    @interface IHXObservable
 */
DECLARE_INTERFACE_( IHXObservable, IUnknown )
{
    /*!
	@function 	AddObserver
	@abstract 	Adds an IHXObserver to this component's list of observers
	@param 		pIObserver [in] The object which wishes to observe this component.
	@result 	Returns HXR_OK on success.  An appropriate error code otherwise.
     */
    STDMETHOD(AddObserver) (THIS_ IHXObserver* pIObserver) PURE;

    /*!
	@function 	RemoveObserver
	@abstract 	Removes an IHXObserver from this component's list of observers
	@param 		pIObserver [in] The object which has been observing this component.
	@result 	Returns HXR_OK on success.  An appropriate error code otherwise.
     */
    STDMETHOD(RemoveObserver) (THIS_ IHXObserver* pIObserver) PURE;
};

DEFINE_SMART_PTR(IHXObservable);

// $EndPrivate.



#endif //_IHXOBSERVABLE_H
