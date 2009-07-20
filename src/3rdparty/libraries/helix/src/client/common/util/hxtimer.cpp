/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: hxtimer.cpp,v 1.4 2007/07/06 21:58:07 jfinnecy Exp $
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

#include "hxtimer.h"

#include "ihxtimerobserver.h"
#include "hxccf.h"


CRNTimer::~CRNTimer( void )
{
    if ( m_pIScheduler && m_hScheduler )
    {
	m_pIScheduler->Remove( m_hScheduler );
	m_hScheduler = 0;
    }
    HX_RELEASE( m_pIScheduler );
}

CRNTimer::CRNTimer( void ) :
    m_pIScheduler( NULL ),
    m_hScheduler( 0 ),
    m_NumPings( IHXTimer::kInvalidTimerParam ),
    m_PingDuration( 0 ),
    m_CurrentPing( 0 )
{
}


BEGIN_COMPONENT_INTERFACE_LIST( CRNTimer )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXContextUser )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXCallback )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXObservable )
    INTERFACE_LIST_ENTRY_SIMPLE( IHXTimer )
END_INTERFACE_LIST

IMPLEMENT_UNMANAGED_COMPONENT (CRNTimer);



// ----------------------------------------------- IHXContextUser 
STDMETHODIMP
CRNTimer::RegisterContext( IUnknown* pContext )
{
    PRE_REQUIRE_RETURN( pContext, HXR_INVALID_PARAMETER );
    
    HX_RESULT res = HXR_FAIL;
    
    SPIHXCommonClassFactory spIObjectSource = pContext;
    if ( spIObjectSource.IsValid() )
    {
	res = spIObjectSource->QueryInterface( IID_IHXScheduler, ( void** ) &m_pIScheduler );
    }
    
    return res;
}


//---------------------------------- IHXObservable 
STDMETHODIMP
CRNTimer::AddObserver( IHXObserver* pObserver )
{
    PRE_REQUIRE_RETURN( pObserver, HXR_INVALID_PARAMETER );

    AddIUnknownToList( pObserver );

    return HXR_OK;
}

STDMETHODIMP
CRNTimer::RemoveObserver( IHXObserver* pObserver )
{
    PRE_REQUIRE_RETURN( pObserver, HXR_INVALID_PARAMETER );

    RemoveIUnknownFromList( pObserver );

    return HXR_OK;
}


// ----------------------------------------------------------------- IHXTimer
STDMETHODIMP 
CRNTimer::SetInfiniteTimer( THIS_ UINT32 pingIntervalMs )
{
    m_PingDuration = pingIntervalMs;
    m_NumPings = IHXTimer::kInvalidTimerParam;
    
    return HXR_OK;
}

STDMETHODIMP 
CRNTimer::SetFiniteTimerIntervalNumber( THIS_ UINT32 pingIntervalMs, UINT32 numPings )
{
    m_PingDuration = pingIntervalMs;
    m_NumPings = numPings;
    m_CurrentPing = 0;
    
    return HXR_OK;
}

STDMETHODIMP 
CRNTimer::SetFiniteTimerDurationInterval( THIS_ UINT32 totalDurationMs, UINT32 pingIntervalMs )
{
    PRE_REQUIRE_RETURN( ( pingIntervalMs > 0 ), HXR_INVALID_PARAMETER );

    m_PingDuration = pingIntervalMs;
    m_NumPings = totalDurationMs / pingIntervalMs;
    m_CurrentPing = 0;

    return HXR_OK;
}

STDMETHODIMP 
CRNTimer::SetFiniteTimerNumberDuration( THIS_ UINT32 numPings, UINT32 totalDurationMs )
{
    PRE_REQUIRE_RETURN( ( numPings > 0 ), HXR_INVALID_PARAMETER );

    m_NumPings = numPings;
    m_PingDuration = totalDurationMs / numPings;
    m_CurrentPing = 0;

    return HXR_OK;
}

STDMETHODIMP 
CRNTimer::GetPingInterval( THIS_ UINT32* pPingIntervalMs ) CONSTMETHOD
{
    PRE_REQUIRE_RETURN( pPingIntervalMs, HXR_INVALID_PARAMETER );

    *pPingIntervalMs = m_PingDuration;
    
    return HXR_OK;
}

STDMETHODIMP 
CRNTimer::StartTimer( THIS )
{
    HX_RESULT outResult = HXR_OK;

    // Can only start timer if timer is not active.
    if ( !m_hScheduler )
    {
	REQUIRE_RETURN( m_pIScheduler, HXR_UNEXPECTED );
	
	outResult = HXR_FAIL;

	// XXXSEH: What if the timer has a ping duration of 0?
	if (
		( IHXTimer::kInvalidTimerParam == m_NumPings )	// infinite timer
	    ||	( m_CurrentPing < m_NumPings )			// finite timer, ping count < max pings
	)
	{
	    // 'this' must be an IRMACallback*.
	    m_hScheduler = m_pIScheduler->RelativeEnter( this, m_PingDuration );
	    if ( m_hScheduler )
	    {
		CRNTimerStartedObserverFunctor theTimerStartedObserverFunctor( this );
		IterateOverIUnknowns( theTimerStartedObserverFunctor );

		outResult = HXR_OK;
	    }
	}
    }
    return outResult;
}

STDMETHODIMP 
CRNTimer::StopTimer( THIS )
{
    HX_RESULT outResult = HXR_OK;

    // Can only stop the timer if it is currently active.
    if ( m_hScheduler )
    {
	REQUIRE_RETURN( m_pIScheduler, HXR_UNEXPECTED );
	
	outResult = m_pIScheduler->Remove( m_hScheduler );
	if ( SUCCEEDED( outResult ) )
	{
	    m_hScheduler = 0;

	    CRNTimerStoppedObserverFunctor theTimerStoppedObserverFunctor( this );
	    IterateOverIUnknowns( theTimerStoppedObserverFunctor );
	}
    }
    return outResult;
}

//xxxsl: This function assumes that it won't be called for an infinite timer that has been stopped.
// In such a case, the result would be that observers would not be notified of the resettimer event.
STDMETHODIMP 
CRNTimer::ResetTimer( THIS )
{
    HX_RESULT outResult = HXR_OK;

    if ( m_hScheduler )
    {
	// Timer is currently active.

	REQUIRE_RETURN( m_pIScheduler, HXR_UNEXPECTED );

	// Reset ping count.
	m_CurrentPing = 0;

	// Stop the timer.
	outResult = m_pIScheduler->Remove( m_hScheduler );
	if ( SUCCEEDED( outResult ) )
	{
	    // Start the timer.
	    m_hScheduler = m_pIScheduler->RelativeEnter( this, m_PingDuration );
	    if ( m_hScheduler )
	    {
		CRNTimerResetObserverFunctor theTimerResetObserverFunctor( this );
		IterateOverIUnknowns( theTimerResetObserverFunctor );
	    }
	    else
	    {
		outResult = HXR_FAIL;
	    }
	}
    }
    else if ( 0 != m_CurrentPing )
    {
	// Timer is not active, and current ping count is not 0.

	// Reset ping count.
	m_CurrentPing = 0;

	CRNTimerResetObserverFunctor theTimerResetObserverFunctor( this );
	IterateOverIUnknowns( theTimerResetObserverFunctor );
    }
    return outResult;
}

STDMETHODIMP_( HXBOOL )
CRNTimer::IsTimerActive( THIS ) CONSTMETHOD
{
    return ( 0 != m_hScheduler );
}



// ---------------------------------------------------------- IRMACallback
STDMETHODIMP
CRNTimer::Func( THIS )
{
    PRE_REQUIRE_RETURN( m_pIScheduler, HXR_UNEXPECTED );
    PRE_REQUIRE_RETURN( m_hScheduler, HXR_UNEXPECTED );

    HX_RESULT outResult = HXR_OK;

    m_hScheduler = 0;
    UINT32 currentPing = ++m_CurrentPing;

    if (
	    ( IHXTimer::kInvalidTimerParam == m_NumPings )  // infinite timer
	||
	    ( currentPing < m_NumPings )		    // finite timer, ping count < max pings
    )
    {
	m_hScheduler = m_pIScheduler->RelativeEnter( this, m_PingDuration );
	if ( !m_hScheduler )
	{
	    outResult = HXR_FAIL;
	}
    }
    HXBOOL isLastPing = ( 0 == m_hScheduler );

    CRNTimerPingedObserverFunctor theTimerPingedObserverFunctor( this, currentPing );
    IterateOverIUnknowns( theTimerPingedObserverFunctor );
    
    // If this was the last ping and the timer has not been restarted by the observer.
    if ( isLastPing && !m_hScheduler )
    {
	CRNTimerStoppedObserverFunctor theTimerStoppedObserverFunctor( this );
	IterateOverIUnknowns( theTimerStoppedObserverFunctor );
	
	// As long as timer has not been restarted.
	if ( !m_hScheduler )
	{
	    ResetTimer( );
	}
    }
    return outResult;
}


//------------------------------------------------------------------ CRNTimerStartedObserverFunctor 
CRNTimer::CRNTimerStartedObserverFunctor::~CRNTimerStartedObserverFunctor( void )
{
}
CRNTimer::CRNTimerStartedObserverFunctor::CRNTimerStartedObserverFunctor( IHXTimer* pITimer )
{
    HX_ASSERT( pITimer );
    m_spITimer = pITimer;
}
void
CRNTimer::CRNTimerStartedObserverFunctor::operator() ( IUnknown* pIUnknownListElement )
{
    SPIHXTimerObserver spITimerObserver = pIUnknownListElement;
    if ( spITimerObserver.IsValid() )
    {
	spITimerObserver->OnTimerStarted( m_spITimer.Ptr() );
    }
}
HXBOOL
CRNTimer::CRNTimerStartedObserverFunctor::ShouldStopIterating( void )
{
    return FALSE;
}


//------------------------------------------------------------------ CRNTimerStoppedObserverFunctor 
CRNTimer::CRNTimerStoppedObserverFunctor::~CRNTimerStoppedObserverFunctor( void )
{
}
CRNTimer::CRNTimerStoppedObserverFunctor::CRNTimerStoppedObserverFunctor( IHXTimer* pITimer )
{
    HX_ASSERT( pITimer );
    m_spITimer = pITimer;
}
void
CRNTimer::CRNTimerStoppedObserverFunctor::operator() ( IUnknown* pIUnknownListElement )
{
    SPIHXTimerObserver spITimerObserver = pIUnknownListElement;
    if ( spITimerObserver.IsValid() )
    {
	spITimerObserver->OnTimerStopped( m_spITimer.Ptr() );
    }
}
HXBOOL
CRNTimer::CRNTimerStoppedObserverFunctor::ShouldStopIterating( void )
{
    return FALSE;
}


//------------------------------------------------------------------ CRNTimerResetObserverFunctor 
CRNTimer::CRNTimerResetObserverFunctor::~CRNTimerResetObserverFunctor( void )
{
}
CRNTimer::CRNTimerResetObserverFunctor::CRNTimerResetObserverFunctor( IHXTimer* pITimer )
{
    HX_ASSERT( pITimer );
    m_spITimer = pITimer;
}
void
CRNTimer::CRNTimerResetObserverFunctor::operator() ( IUnknown* pIUnknownListElement )
{
    SPIHXTimerObserver spITimerObserver = pIUnknownListElement;
    if ( spITimerObserver.IsValid() )
    {
	spITimerObserver->OnTimerReset( m_spITimer.Ptr() );
    }
}
HXBOOL
CRNTimer::CRNTimerResetObserverFunctor::ShouldStopIterating( void )
{
    return FALSE;
}


//------------------------------------------------------------------ CRNTimerPingedObserverFunctor 
CRNTimer::CRNTimerPingedObserverFunctor::~CRNTimerPingedObserverFunctor( void )
{
}
CRNTimer::CRNTimerPingedObserverFunctor::CRNTimerPingedObserverFunctor( IHXTimer* pITimer, UINT32 pingNumber )
{
    HX_ASSERT( pITimer );
    m_spITimer = pITimer;
    m_PingNumber = pingNumber;
}
void
CRNTimer::CRNTimerPingedObserverFunctor::operator() ( IUnknown* pIUnknownListElement )
{
    SPIHXTimerObserver spITimerObserver = pIUnknownListElement;
    if ( spITimerObserver.IsValid() )
    {
	spITimerObserver->OnTimerPinged( m_spITimer.Ptr(), m_PingNumber );
    }
}
HXBOOL
CRNTimer::CRNTimerPingedObserverFunctor::ShouldStopIterating( void )
{
    return FALSE;
}
