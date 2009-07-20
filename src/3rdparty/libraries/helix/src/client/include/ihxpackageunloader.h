/* ***** BEGIN LICENSE BLOCK *****
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

#ifndef IHXPACKAGEUNLOADER_H_
#define IHXPACKAGEUNLOADER_H_

/*!
    @singletype IHXPackageUnloader
    @abstract IHXPackageUnloader Interface
*/

#include "hxcom.h"
#include "hxcomptr.h"

_INTERFACE IHXTimer;

// $Private;

// {91994294-C0CB-446c-A823-CA60C914C525}
DEFINE_GUID(IID_IHXPackageUnloader, 0x91994294, 0xc0cb, 0x446c, 0xa8, 0x23, 0xca, 0x60, 0xc9, 0x14, 0xc5, 0x25);


#undef INTERFACE
#define INTERFACE IHXPackageUnloader

DECLARE_INTERFACE_ (IHXPackageUnloader, IUnknown)
{
    /*!
	@function   Construct
	@abstract   This method functions as a deferred constructor, and thus should only be called once.
	@param	    pITimer [in] The timer to attach to.
	@param	    pPackage [in] The package name.
	@result     Indicates the success of construction.
    */
    STDMETHOD (Construct) (IHXTimer*, char const*) PURE;

    /*!
	@function   Destruct
	@abstract   This method functions as a pre destructor.
	@result     Indicates the success of destruction.
    */
    STDMETHOD (Destruct) (void) PURE;
};

DEFINE_SMART_PTR(IHXPackageUnloader);

// $EndPrivate.

#endif // header block
