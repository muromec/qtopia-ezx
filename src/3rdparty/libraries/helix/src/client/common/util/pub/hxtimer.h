/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: hxtimer.h,v 1.3 2007/07/06 21:58:08 jfinnecy Exp $
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

#ifndef _CRNTIMER_H_
#define _CRNTIMER_H_

#include "unkimp.h"

#include "hxiunknownobservermanager.h"

#include "ihxtimer.h"
#include "ihxobservable.h"
#include "ihxcontextuser.h"

#include "hxengin.h"
#include "hxcomponent.h"

/*!
    @header CRNTimer.h
    @abstract 
 */

/*!
    @class CRNTimer
 */
class CRNTimer :
    public CUnknownIMP,
    public CHXIUnknownObserverManagerMixin, // implements CUnknownIMP
    public IHXContextUser,
    public IHXCallback,
    public IHXObservable,
    public IHXTimer 
{
    DECLARE_UNMANAGED_COMPONENT (CRNTimer)

public:

    CRNTimer( void );
    virtual ~CRNTimer( void );

    // IHXContextUser
    STDMETHOD (RegisterContext) (THIS_ IUnknown *pContext);

    // IHXCallback
    STDMETHOD ( Func ) ( THIS );

    // IHXObservable
    STDMETHOD ( AddObserver )( THIS_ IHXObserver* pObserver );
    STDMETHOD ( RemoveObserver )( THIS_ IHXObserver* pObserver );

    // IHXTimer
    STDMETHOD ( SetInfiniteTimer ) ( THIS_ UINT32 pingInterval );
    STDMETHOD ( SetFiniteTimerIntervalNumber ) ( THIS_ UINT32 pingIntervalMs, UINT32 numPings );
    STDMETHOD ( SetFiniteTimerDurationInterval ) ( THIS_ UINT32 totalDurationMs, UINT32 pingIntervalMs );
    STDMETHOD ( SetFiniteTimerNumberDuration ) ( THIS_ UINT32 numPings, UINT32 totalDurationMs );
    STDMETHOD ( GetPingInterval ) ( THIS_ UINT32* pPingIntervalMs ) CONSTMETHOD;
    STDMETHOD ( StartTimer ) ( THIS );
    STDMETHOD ( StopTimer ) ( THIS );
    STDMETHOD ( ResetTimer ) ( THIS );
    STDMETHOD_( HXBOOL, IsTimerActive ) ( THIS ) CONSTMETHOD;

protected:

private:

    IHXScheduler*		m_pIScheduler;
    CallbackHandle		m_hScheduler;

    UINT32			m_NumPings;
//xxxsl: m_PingDuration is poorly named.  A ping duration is the amount of time the ping is active.
//	 A better name would have been: m_PingInterval.  A ping interval is the amount of time 
//	 between two consecutive pings.
    UINT32			m_PingDuration;
    UINT32			m_CurrentPing;

    class CRNTimerStartedObserverFunctor : public CHXIUnknownElementFunctor
    {
	public:
	    virtual ~CRNTimerStartedObserverFunctor( void );
	    CRNTimerStartedObserverFunctor( IHXTimer* pITimer );
	    virtual void operator() ( IUnknown* pIUnknownListElement );
	    virtual HXBOOL ShouldStopIterating( void );
	private:
	    SPIHXTimer m_spITimer;
    };

    class CRNTimerStoppedObserverFunctor : public CHXIUnknownElementFunctor
    {
	public:
	    virtual ~CRNTimerStoppedObserverFunctor( void );
	    CRNTimerStoppedObserverFunctor( IHXTimer* pITimer );
	    virtual void operator() ( IUnknown* pIUnknownListElement );
	    virtual HXBOOL ShouldStopIterating( void );
	private:
	    SPIHXTimer m_spITimer;
    };

    class CRNTimerResetObserverFunctor : public CHXIUnknownElementFunctor
    {
	public:
	    virtual ~CRNTimerResetObserverFunctor( void );
	    CRNTimerResetObserverFunctor( IHXTimer* pITimer );
	    virtual void operator() ( IUnknown* pIUnknownListElement );
	    virtual HXBOOL ShouldStopIterating( void );
	private:
	    SPIHXTimer m_spITimer;
    };

    class CRNTimerPingedObserverFunctor : public CHXIUnknownElementFunctor
    {
	public:
	    virtual ~CRNTimerPingedObserverFunctor( void );
	    CRNTimerPingedObserverFunctor( IHXTimer* pITimer, UINT32 pingNumber );
	    virtual void operator() ( IUnknown* pIUnknownListElement );
	    virtual HXBOOL ShouldStopIterating( void );
	private:
	    SPIHXTimer m_spITimer;
	    UINT32 m_PingNumber;
    };

};

#endif // _CRNTimer_H_
