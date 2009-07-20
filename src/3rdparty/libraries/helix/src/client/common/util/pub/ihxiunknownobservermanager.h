/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: ihxiunknownobservermanager.h,v 1.1 2006/03/22 17:51:59 stanb Exp $
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

#ifndef _IHXIUNKNOWNOBSERVERMANAGER_H_
#define _IHXIUNKNOWNOBSERVERMANAGER_H_

#include "hxcom.h"
#include "hxcomptr.h"

/*!
    @singletype	IHXIUnknownObserverManager
    @abstract	Implemented by any object that manages a list of IUnknown IHXObserver components and wish
		to expose these components to external objects.
 */


// {A278C7A4-3A5D-4da0-8486-99F167E9380C}
DEFINE_GUID(IID_IHXIUnknownObserverManager, 0xa278c7a4, 0x3a5d, 0x4da0, 0x84, 0x86, 0x99, 0xf1, 0x67, 0xe9, 0x38, 0xc);

_INTERFACE IHXIUnknownElementFunctor;

// $InDevelopment:


DECLARE_INTERFACE_( IHXIUnknownObserverManager, IUnknown )
{
    /*!
	@function   IterateOverIUnknowns
	@abstract   Called when a component wishes to have the specified functor applied to each object in the
		    IHXObserver list
	@param	    pIUnkFunctor [in] The functor to apply.
     */
    STDMETHOD_( void, IterateOverIUnknowns ) ( THIS_ IHXIUnknownElementFunctor *pIUnkFunctor ) PURE;
}; 

DEFINE_SMART_PTR(IHXIUnknownObserverManager);

// $EndInDevelopment.


#endif // _IHXIUNKNOWNOBSERVERMANAGER_H_
