/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: ihxtimerobserver.h,v 1.2 2007/07/06 21:58:08 jfinnecy Exp $
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

#ifndef _IHX_TIMER_OBSERVER_H_
#define _IHX_TIMER_OBSERVER_H_

/*!
    @singletype IHXTimerObserver
    @abstract Receives notifications from a timer
*/

#include "hxcom.h"
#include "hxcomptr.h"
#include "ihxobserver.h"

// $Private;

// {B788E1D2-9C73-4e96-88D4-F3F2FF84B857}
DEFINE_GUID(IID_IHXTimerObserver, 0xb788e1d2, 0x9c73, 0x4e96, 0x88, 0xd4, 0xf3, 0xf2, 0xff, 0x84, 0xb8, 0x57);

#undef INTERFACE
#define INTERFACE IHXTimerObserver

_INTERFACE IHXTimer;

DECLARE_INTERFACE_( IHXTimerObserver, IHXObserver )
{
    /*!
	@function   OnTimerStarted
	@abstract   Called when an observed timer has started
	@param	    pITimer [in] The timer associated with this callback.
    */
    STDMETHOD_( void, OnTimerStarted ) ( THIS_ IHXTimer* pITimer ) PURE;

    /*!
	@function   OnTimerStopped
	@abstract   Called when an observed timer has stopped
	@param	    pITimer [in] The timer associated with this callback.
    */
    STDMETHOD_( void, OnTimerStopped ) ( THIS_ IHXTimer* pITimer ) PURE;

    /*!
	@function   OnTimerReset
	@abstract   Called when an observed timer has been reset
	@param	    pITimer [in] The timer associated with this callback.
    */
    STDMETHOD_( void, OnTimerReset ) ( THIS_ IHXTimer* pITimer ) PURE;

    /*!
	@function   OnTimerPinged
	@abstract   Called when an observed timer has fired
	@param	    pITimer [in] The timer associated with this callback.
	@param	    tickNumber [in] The current ping associated with this callback.
    */
    STDMETHOD_( void, OnTimerPinged ) ( THIS_ IHXTimer* pITimer, UINT32 pingNumber ) PURE;
};

DEFINE_SMART_PTR(IHXTimerObserver);

// $EndPrivate.



#endif // _IHX_TIMER_OBSERVER_H_
