/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: ihxtimer.h,v 1.4 2007/07/06 21:58:08 jfinnecy Exp $
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

#ifndef _IHX_TIMER_H_
#define _IHX_TIMER_H_

/*!
    @singletype IHXTimer
    @abstract IHXTimer Interface
*/

#include "hxcom.h"
#include "hxcomptr.h"

// $Private;

// {97800EDD-6E7F-4c91-906D-47DE0B0592F5}
DEFINE_GUID(IID_IHXTimer, 0x97800edd, 0x6e7f, 0x4c91, 0x90, 0x6d, 0x47, 0xde, 0xb, 0x5, 0x92, 0xf5);

#undef INTERFACE
#define INTERFACE IHXTimer


DECLARE_INTERFACE_( IHXTimer, IUnknown )
{
    enum
    {
	kInvalidTimerParam = -1
    };

    /*!
	@function   SetInfiniteTimer
	@abstract   This method is intended to setup a timer for use.  If this method
		    is called while the timer is running, the timer will change it's
		    interval and duration to match the new settings.
		    No stop and start of the timer will be done/observed by using
		    this method.
	@param	    pingInterval [in] The amount of time taken for each ping.
	@result     HX_RESULT Indicates success setting up an infinite timer.
    */
    STDMETHOD ( SetInfiniteTimer ) ( THIS_ UINT32 pingInterval ) PURE;

    /*!
	@function   SetFiniteTimerIntervalNumber
	@abstract   This method is intended to setup a timer for use.  If this method
		    is called while the timer is running, the timer will change it's
		    interval and duration to match the new settings.
		    No stop and start of the timer will be done/observed by using
		    this method.
	@param	    pingIntervalMs [in] The amount of time taken for each ping (ms).
	@param	    numPings [in] The number of timer pings that you would like to get.
	@result     HX_RESULT Indicates success in setting up a finite timer with ping interval
		    and number of pings.
    */
    STDMETHOD ( SetFiniteTimerIntervalNumber ) ( THIS_ UINT32 pingIntervalMs, UINT32 numPings ) PURE;

    /*!
	@function   SetFiniteTimerDurationInterval
	@abstract   This method is intended to setup a timer for use.  If this method
		    is called while the timer is running, the timer will change it's
		    interval and duration to match the new settings.
		    No stop and start of the timer will be done/observed by using
		    this method.
	@param	    totalDurationMs [in] The amount of total time to send ping callbacks (ms).
	@param	    pingIntervalMs [in] The amount of time taken for each ping (ms).
	@result     HX_RESULT Indicates success in setting up a finite timer with ping interval
		    and the total duration of the timer.
    */
    STDMETHOD ( SetFiniteTimerDurationInterval ) ( THIS_ UINT32 totalDurationMs, UINT32 pingIntervalMs ) PURE;

    /*!
	@function   SetFiniteTimerNumberDuration
	@abstract   This method is intended to setup a timer for use.  If this method
		    is called while the timer is running, the timer will change it's
		    interval and duration to match the new settings.
		    No stop and start of the timer will be done/observed by using
		    this method.
	@param	    numPings [in] The number of timer pings that you would like to get.
	@param	    totalDurationMs [in] The amount of total time to send ping callbacks (ms).
	@result     HX_RESULT Indicates success in setting up a finite timer with the number of pings
		    and the total duration of the timer.
    */
    STDMETHOD ( SetFiniteTimerNumberDuration ) ( THIS_ UINT32 numPings, UINT32 totalDurationMs ) PURE;

    /*!
	@function   GetPingInterval
	@abstract   Get timer information
	@param	    pPingIntervalMs [out] The amount of time taken for each ping.
	@result     HX_RESULT Indicates success in getting the ping interval time (ms).
    */
    STDMETHOD ( GetPingInterval ) ( THIS_ UINT32* pPingIntervalMs ) CONSTMETHOD PURE;

    /*!
	@function   StartTimer
	@abstract Start the timer. Note that if the timer has been reset this will 
	  result in an instant ping.
	@result		HXR_OK on success.	  
    */
    STDMETHOD ( StartTimer ) ( THIS ) PURE;

    /*!
	@function   StopTimer
	@abstract   Stop the timer
	@result	    HXR_OK on success.	
    */
    STDMETHOD ( StopTimer ) ( THIS ) PURE;

    /*!
	@function   ResetTimer
	@abstract   Causes the timer to restart the current interval
	@result		HXR_OK on success.	
    */
    STDMETHOD ( ResetTimer ) ( THIS ) PURE;

    /*!
	@function   IsTimerActive
	@abstract   Get timer state.
	@result     TRUE if the timer is currently running.
    */
    STDMETHOD_( HXBOOL, IsTimerActive ) ( THIS ) CONSTMETHOD PURE;

};

DEFINE_SMART_PTR(IHXTimer);

// $EndPrivate.



#endif // _IHX_TIMER_H_
