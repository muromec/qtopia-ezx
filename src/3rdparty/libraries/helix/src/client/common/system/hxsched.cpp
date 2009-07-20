/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsched.cpp,v 1.37 2009/05/05 16:39:12 sfu Exp $
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

#include "hxcom.h"
#include "hxtypes.h"
#include "hxresult.h"
#include "hxthreadyield.h"
#ifdef _WINDOWS
#include "hlxclib/windows.h"
#endif
#include "hlxclib/stdlib.h"
#include "timeval.h"
#include "clientpq.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxprefutil.h"
#include "timeline.h"
#include "hxtick.h"
#include "thrdutil.h"
#ifdef _MACINTOSH
#include "hx_moreprocesses.h"
#endif

#if defined(_WIN32) || defined(THREADS_SUPPORTED)
#include "casyntim.h"
#endif /*_WIN32*/

#include "hxthread.h"

#include "hxsched.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if defined(_WINDOWS) && !defined(_WIN32)   /* WIN16 */
#define MINIMUM_GRANULARITY   55
#elif defined (_MACINTOSH)		    /* MACINTOSH */
#define MINIMUM_GRANULARITY   20
#elif defined (_UNIX)
#define MINIMUM_GRANULARITY   5
#elif defined (_SYMBIAN)
#define MINIMUM_GRANULARITY   20
#elif defined (_BREW)
#define MINIMUM_GRANULARITY   30
#else					    /* ELSE */
#define MINIMUM_GRANULARITY   5
#endif 

#define MINIMUM_DIFFERENCE    5

// HXScheduler...
HXScheduler::HXScheduler(IUnknown* pContext) :
     m_lRefCount (0)
    ,m_pScheduler(0)
    ,m_bUseDeferredTask(TRUE)
    ,m_pInterruptTimeScheduler(0)
    ,m_pInterruptTimeOnlyScheduler(0)
    ,m_pID(0)
    ,m_pPQMutex(NULL)
    ,m_pContext(pContext)
    ,m_bLocked(FALSE)
    ,m_ulLastUpdateTime(0)
    ,m_pCoreMutex(NULL)
    ,m_bIsInterruptEnabled(FALSE)
    ,m_headTime(0)
    ,m_interruptHeadTime(0)
    ,m_interruptOnlyHeadTime(0)
    ,m_ulSystemNextDueTime(0)
    ,m_ulInterruptNextDueTime(0)
    ,m_ulInterruptOnlyNextDueTime(0)
    ,m_bImmediatesPending(0)

#if defined(_WIN32) || defined(THREADS_SUPPORTED)
    ,m_pAsyncTimer(0)
#endif 
    ,m_pTimeline(0)
    ,m_ulCurrentGranularity(0)
    ,m_ulMinimumGranularity(MINIMUM_GRANULARITY)
    ,m_pWaitEvent(NULL)
    ,m_bWaitPending(FALSE)
    ,m_bWaitedEventFired(FALSE)
    ,m_ulThreadID(0)
{
    // obtain class factory
    IHXCommonClassFactory* pCCF = NULL;
    if (m_pContext)
    {
        m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**) &pCCF);
    }

    // create priority queue mutex
    if (pCCF)
    {
        pCCF->CreateInstance(CLSID_IHXMutex, (void**)&m_pPQMutex);
    }

    // create priority queues
    m_pID = new CHXID(100);
    m_pScheduler = new ClientPQ(pContext, m_pID, m_pPQMutex);
    m_pInterruptTimeScheduler = new ClientPQ(pContext, m_pID, m_pPQMutex);
    m_pInterruptTimeOnlyScheduler = new ClientPQ(pContext, m_pID, m_pPQMutex);

    // Create event that wil be used for scheduler waiting operation
    if (pCCF)
    {
        pCCF->CreateInstance(IID_IHXEvent, (void**) &m_pWaitEvent);

	if (m_pWaitEvent)
	{
	    if (FAILED(m_pWaitEvent->Init("Scheduler_Wait", FALSE)))
	    {
		HX_RELEASE(m_pWaitEvent);
	    }
	}

	ReadPrefUINT32(m_pContext, "SchedulerMinimumGranularity", m_ulMinimumGranularity);
    }

    // release class factory
    HX_RELEASE(pCCF);

    (void) gettimeofday(&m_CurrentTimeVal, 0);
    m_ulLastUpdateTime = HX_GET_TICKCOUNT();
    m_ulThreadID = HXGetCurrentThreadID();

}

HXScheduler::~HXScheduler()
{
    StopScheduler();

    HX_DELETE(m_pScheduler);
    HX_DELETE(m_pInterruptTimeScheduler);
    HX_DELETE(m_pInterruptTimeOnlyScheduler);
    HX_DELETE(m_pID);
    HX_RELEASE(m_pPQMutex);

#if defined(_WIN32) || defined(THREADS_SUPPORTED)
    HX_DELETE(m_pAsyncTimer);
#endif 
    HX_DELETE(m_pTimeline);
    HX_RELEASE(m_pWaitEvent);
    HX_RELEASE(m_pCoreMutex);
}

/*
 * IUnknown methods
 */



/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your 
//		object.
//
STDMETHODIMP HXScheduler::QueryInterface(REFIID riid, void** ppvObj)
{
#ifdef _MACINTOSH
    if (IsEqualIID(riid, IID_IHXInterruptState) && m_pContext)
    {
	return m_pContext->QueryInterface(riid, ppvObj);
    }
#endif

    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXScheduler), (IHXScheduler*)this },
            { GET_IIDHANDLE(IID_IHXScheduler2), (IHXScheduler2*)this },
            { GET_IIDHANDLE(IID_IHXScheduler3), (IHXScheduler3*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXScheduler*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXScheduler::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::Release
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXScheduler::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}


/*
 * HXScheduler methods
 */
CallbackHandle HXScheduler::_AbsoluteEnter(IHXCallback* pCallback, 
					   Timeval tVal)
{
    CallbackHandle hCallback;

    if (pCallback)
    {
	IHXInterruptSafe* pInterruptSafeCB = NULL;
	IHXInterruptOnly* pInterruptOnlyCB = NULL;

	if ((HXR_OK == pCallback->QueryInterface(IID_IHXInterruptSafe, 
					        (void**) &pInterruptSafeCB)) &&
	    (pInterruptSafeCB->IsInterruptSafe()))
	{
	    hCallback = (CallbackHandle) m_pInterruptTimeScheduler->
    						    enter(tVal, pCallback);
	}
	else if ((HXR_OK == pCallback->QueryInterface(IID_IHXInterruptOnly, 
					             (void**) &pInterruptOnlyCB)) &&
		 (pInterruptOnlyCB->IsInterruptOnly()))
	{
	    hCallback = (CallbackHandle) m_pInterruptTimeOnlyScheduler->
    						    enter(tVal, pCallback);
	}
	else
	{
	    hCallback = (CallbackHandle) m_pScheduler->enter(tVal, pCallback);
	}

	HX_RELEASE(pInterruptSafeCB);
	HX_RELEASE(pInterruptOnlyCB);

	if (m_bWaitPending && m_pWaitEvent)
	{
	    // Signal to the waiting routine that a new entry has
	    // been made and wait should be re-evaluated.
	    m_pWaitEvent->SignalEvent();
	}
    }

    return hCallback;
}

/************************************************************************
 *	Method:
 *		IHXScheduler::Enter
 *	Purpose:
 *		enter objects in the service queue
 */
STDMETHODIMP_(CallbackHandle)
HXScheduler::RelativeEnter(IHXCallback* pCallback, ULONG32 ulTime)
{
    Timeval absVal;

    /*
     * A RelativeEnter() of 0 ms is a special case that needs to be
     * AbsoluteEnter() of 0
     */
    if (ulTime == 0)
    {
	absVal.tv_sec = absVal.tv_usec = 0;
    }
    else
    {
	UINT32  usecs = 0;
	UINT32  secs = 0;
	Timeval lTime;

	// handle the possible overflow of UINT32 when
	// converting from milli-second to micro-second
	if (ulTime > 4000000)
	{
	    secs = ulTime / 1000;
	    usecs = (ulTime % 1000) * 1000;
	}
	else
	{
	    secs = 0;
	    usecs = ulTime * 1000;

	    if (usecs >= 1000000)
	    {
		secs = usecs / 1000000;
		usecs = usecs % 1000000;
	    }
	}

	lTime.tv_sec = secs;
	lTime.tv_usec = usecs;

	absVal.tv_sec = m_CurrentTimeVal.tv_sec;
	absVal.tv_usec = m_CurrentTimeVal.tv_usec;
	absVal += lTime;
    }

    return _AbsoluteEnter(pCallback, absVal);
}

/************************************************************************
 *	Method:
 *	    IHXScheduler::AbsoluteEnter
 *	Purpose:
 *	    enter objects in the service queue at absolute time
 */
STDMETHODIMP_(CallbackHandle)
HXScheduler::AbsoluteEnter(IHXCallback* pCallback, HXTimeval tVal)
{
    Timeval timeVal;

    timeVal.tv_sec = tVal.tv_sec;
    timeVal.tv_usec = tVal.tv_usec;

    return _AbsoluteEnter(pCallback, timeVal);
}

/************************************************************************
 *	Method:
 *		IHXScheduler::Remove
 *	Purpose:
 *		remove objects from the service queue
 */
STDMETHODIMP HXScheduler::Remove(CallbackHandle Handle)
{
    if (m_pInterruptTimeScheduler->removeifexists(Handle))
    {
    	return HXR_OK;
    }
    else if (m_pInterruptTimeOnlyScheduler->removeifexists(Handle))
    {
    	return HXR_OK;
    }

    m_pScheduler->remove(Handle);
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXScheduler::GetCurrentSchedulerTime
 *	Purpose:
 *	    gives the current time in the timeline of the scheduler...
 */
STDMETHODIMP_(HXTimeval) HXScheduler::GetCurrentSchedulerTime(void)
{
    HXTimeval hxTimeval;
    hxTimeval.tv_sec = m_CurrentTimeVal.tv_sec;
    hxTimeval.tv_usec = m_CurrentTimeVal.tv_usec;
    return hxTimeval;
}

UINT32 HXScheduler::_Execute(ClientPQ* pScheduler, HXBOOL &bImmediatesPending)
{
    int count = 0;

    bImmediatesPending = FALSE;

    if (!pScheduler->empty())
    {
	count = pScheduler->execute(m_CurrentTimeVal);

	if (count != 0)
	{
	    if (m_bWaitPending && m_pWaitEvent)
	    {
		m_bWaitedEventFired = TRUE;
		m_pWaitEvent->SignalEvent();
	    }
	}

	int immCount = 0;

	/*
	* Don't execute more then 100 immediate elements.  We don't wanna
	* hold on too long and spin here.
	*/
	// Keep executing until there are no more zero time elements
	while ((bImmediatesPending = pScheduler->immediate()) && 
	       (immCount < 100))
	{
	    immCount += pScheduler->execute(m_CurrentTimeVal);
	}

	count += immCount;
    }

    return ((UINT32) count);
}

UINT32 HXScheduler::_GetNextDueTimeInMs(ClientPQ* pScheduler)
{
    UINT32 ulNextDueTimeInMs;

    if (!pScheduler->empty())
    {
        Timeval timeout(0,0);
        Timeval tvHeadTime = pScheduler->head_time();
        if (tvHeadTime >= m_CurrentTimeVal)
        {
            timeout = tvHeadTime - m_CurrentTimeVal;
        }
        ulNextDueTimeInMs = (UINT32) (timeout.tv_sec * 1000 + 
				      timeout.tv_usec / 1000);
    }
    else
    {
	ulNextDueTimeInMs = m_ulCurrentGranularity;
    }

    return ulNextDueTimeInMs;
}

HX_RESULT HXScheduler::ExecuteCurrentFunctions(HXBOOL bAtInterrupt)
{
    HXBOOL bShouldServiceSystem	= FALSE;
    HXBOOL bShouldServiceInterrupt = FALSE;
    HXBOOL bShouldServiceInterruptOnly = FALSE;
    HXBOOL bImmediatesPending = FALSE;
    HXBOOL bInterruptImmediatesPending = FALSE;
    HXBOOL bInterruptOnlyImmediatesPending = FALSE;
    ULONG32 startime;
    StartYield(&startime);
    HXBOOL bUpdateRet = UpdateCurrentTime(bAtInterrupt, 
					  bShouldServiceSystem, 
					  bShouldServiceInterrupt,
					  bShouldServiceInterruptOnly);
    if (!bUpdateRet)
    {
        return HXR_OK;
    }

    /* if not at interrupt time, execute interrupt safe & 
     * non-interrupt safe tasks 
     */
    if (bShouldServiceInterrupt)
    {
#ifdef HELIX_CONFIG_COOP_MULTITASK    
        m_pInterruptTimeScheduler->m_lastTime = startime;
#endif
	_Execute(m_pInterruptTimeScheduler, bInterruptImmediatesPending);
#ifdef HELIX_CONFIG_COOP_MULTITASK   
	startime = m_pInterruptTimeScheduler->m_lastTime;
#endif
    }

    if (bShouldServiceInterruptOnly)
    {
#ifdef HELIX_CONFIG_COOP_MULTITASK      
        m_pInterruptTimeOnlyScheduler->m_lastTime = startime;
#endif
	_Execute(m_pInterruptTimeOnlyScheduler, bInterruptOnlyImmediatesPending);
#ifdef HELIX_CONFIG_COOP_MULTITASK  
	startime = m_pInterruptTimeOnlyScheduler->m_lastTime;
#endif
    }

    if (bShouldServiceSystem)
    {
#ifdef HELIX_CONFIG_COOP_MULTITASK      
       m_pScheduler->m_lastTime = startime;	   
#endif
	UINT32 ulElemsExecuted = _Execute(m_pScheduler, bImmediatesPending);
#ifdef HELIX_CONFIG_COOP_MULTITASK  
	startime = m_pScheduler->m_lastTime;
#endif
	
#if defined(_WIN32) || defined(THREADS_SUPPORTED)
	if (ulElemsExecuted != 0)
	{
	    HXBOOL bChange = FALSE;
	    if (m_pScheduler->empty())
	    {
		if (m_ulCurrentGranularity < m_ulMinimumGranularity &&
		    (m_ulMinimumGranularity - m_ulCurrentGranularity >= 
		     MINIMUM_DIFFERENCE))
		{
		    bChange = TRUE;
		    m_ulCurrentGranularity = m_ulMinimumGranularity;
		}
	    }
	    else if (m_ulCurrentGranularity > MINIMUM_DIFFERENCE)
	    {
		Timeval timeout	  = m_pScheduler->head_time() - m_CurrentTimeVal;
		INT32  lTimeoutInMs;

		if (timeout.tv_sec >= 0)
		{
		    lTimeoutInMs = timeout.tv_sec * 1000 + 
			timeout.tv_usec / 1000;
		}
		else
		{
		    lTimeoutInMs = 0;
		}

		if (lTimeoutInMs > 0 &&
		    ((UINT32) lTimeoutInMs) < m_ulCurrentGranularity &&
		    (m_ulCurrentGranularity - (UINT32) lTimeoutInMs >= 
		     MINIMUM_DIFFERENCE))
		{
		    bChange = TRUE;
		    m_ulCurrentGranularity = (UINT32) ((lTimeoutInMs >= MINIMUM_DIFFERENCE ? lTimeoutInMs: MINIMUM_DIFFERENCE));
		}
	    }

	    if (bChange)
	    {
		m_pTimeline->Pause();
		/* Reset the granularity */
		m_pTimeline->SetGranularity(m_ulCurrentGranularity);
		/* Resume */
		m_pTimeline->Resume();
	    }
	}
#endif /*_WIN32*/
    }

    m_bImmediatesPending = (bImmediatesPending || 
			    bInterruptImmediatesPending || 
			    bInterruptOnlyImmediatesPending);

    m_ulSystemNextDueTime = _GetNextDueTimeInMs(m_pScheduler);
    m_ulInterruptNextDueTime = _GetNextDueTimeInMs(m_pInterruptTimeScheduler);
    m_ulInterruptOnlyNextDueTime = _GetNextDueTimeInMs(m_pInterruptTimeOnlyScheduler);

    return HXR_OK;
}

HXBOOL HXScheduler::IsEmpty() 
{
    return (m_pScheduler->empty() && 
	    m_pInterruptTimeScheduler->empty() &&
	    m_pInterruptTimeOnlyScheduler->empty());
}

STDMETHODIMP_(HXBOOL)
HXScheduler::GetNextEventDueTimeDiff(ULONG32 &ulEarliestDueTimeDiff)
{
    if (m_pScheduler->empty() && 
	m_pInterruptTimeScheduler->empty() &&
	m_pInterruptTimeOnlyScheduler->empty())
    {
	return FALSE;
    }

    Timeval nextDueTime;
    Timeval now;
    
    GetTime(&now);

    if (m_pScheduler->empty())
    {
	if (m_pInterruptTimeOnlyScheduler->empty())
	{
	    nextDueTime = m_pInterruptTimeScheduler->head_time();
	}
	else
	{
	    if (m_pInterruptTimeScheduler->head_time() < m_pInterruptTimeOnlyScheduler->head_time())
	    {
		nextDueTime = m_pInterruptTimeScheduler->head_time();
	    }
	    else
	    {
		nextDueTime = m_pInterruptTimeOnlyScheduler->head_time();
	    }
	}
    }
    else if (m_pInterruptTimeScheduler->empty())
    {
	if (m_pInterruptTimeOnlyScheduler->empty())
	{
	    nextDueTime = m_pScheduler->head_time();
	}
	else
	{
	    if (m_pScheduler->head_time() < m_pInterruptTimeOnlyScheduler->head_time())
	    {
		nextDueTime = m_pScheduler->head_time();
	    }
	    else
	    {
		nextDueTime = m_pInterruptTimeOnlyScheduler->head_time();
	    }
	}
    }
    else if (m_pInterruptTimeOnlyScheduler->empty())
    {
	if (m_pInterruptTimeScheduler->empty())
	{
	    nextDueTime = m_pScheduler->head_time();
	}
	else
	{
	    if (m_pScheduler->head_time() < m_pInterruptTimeScheduler->head_time())
	    {
		nextDueTime = m_pScheduler->head_time();
	    }
	    else
	    {
		nextDueTime = m_pInterruptTimeScheduler->head_time();
	    }
	}
    }
    else
    {
	if (m_pInterruptTimeScheduler->head_time() < m_pScheduler->head_time())
	{
	    nextDueTime = m_pInterruptTimeScheduler->head_time();
	}
	else
	{
	    nextDueTime = m_pScheduler->head_time();
	}

	if (m_pInterruptTimeOnlyScheduler->head_time() < nextDueTime)
	{
	    nextDueTime = m_pInterruptTimeOnlyScheduler->head_time();
	}
    }


    if (nextDueTime > now)
    {
	nextDueTime -= now;
    }
    else
    {
	nextDueTime = 0.0;
    }

    ulEarliestDueTimeDiff = (ULONG32) (	nextDueTime.tv_sec*1000 + 
					nextDueTime.tv_usec/1000);

    return TRUE;
}

// Current implementation supports only one WaitForNextEvent
// at the time (single-threaded use).
STDMETHODIMP
HXScheduler::WaitForNextEvent(ULONG32 ulTimeout)
{
    HX_RESULT retVal = HXR_FAIL;
    UINT32 ulTimeToNextEvent = ALLFS;
    UINT32 ulWaitStartTime = 0;

    if (m_pWaitEvent)
    {
	if (m_bWaitPending)
	{
	    return HXR_UNEXPECTED;
	}

	retVal = HXR_OK;
	m_pWaitEvent->ResetEvent();
	m_bWaitedEventFired = FALSE;
	m_bWaitPending = TRUE;
	do
	{
	    GetNextEventDueTimeDiff(ulTimeToNextEvent);
	    if (ulTimeout > ulTimeToNextEvent)
	    {
		ulTimeout = ulTimeToNextEvent;
	    }
	    if ((ulTimeout != 0) && (!m_bWaitedEventFired))
	    {
                UINT32 ulWaitTime;

                if (ulWaitStartTime == 0)
                {
                    ulWaitStartTime  = HX_GET_BETTERTICKCOUNT();
                }

		retVal = m_pWaitEvent->Wait(ulTimeout);

                ulWaitTime = HX_GET_BETTERTICKCOUNT() - ulWaitStartTime;
                if (ulWaitTime < ulTimeout)
                {
                    ulTimeout -= ulWaitTime;
                }
                else
                {
                    ulTimeout = 0;
                }
	    }
	} while ((!m_bWaitedEventFired) &&
		 (ulTimeout != 0) &&
		 ((retVal == HXR_OK) ||
		  (retVal == HXR_WOULD_BLOCK)));

	// If the status is normal, determine if waited event
	// occured or not.  HXR_WOULD_BLOCK is returned on wait
	// timeout.
	if ((retVal == HXR_OK) ||
	    (retVal == HXR_WOULD_BLOCK))
	{
	    retVal = HXR_WOULD_BLOCK;

	    // The event occured if it fired or if its due time
	    // was reached but event did not fire yet.
	    // We cannot expect event to fire in cases where
	    // events can fire only on main scheduler thread
	    // as wait call is blocking this thread.
	    if (m_bWaitedEventFired ||
		(ulTimeToNextEvent == 0))
	    {
		retVal = HXR_OK;
	    }
	}

	m_bWaitPending = FALSE;
    }

    return retVal;
}

STDMETHODIMP_(ULONG32)
HXScheduler::GetThreadID(void)
{
    return m_ulThreadID;
}

STDMETHODIMP_(HXBOOL)
HXScheduler::AreImmediatesPending(void)
{
    return m_bImmediatesPending; 
}

STDMETHODIMP
HXScheduler::StartScheduler()
{
    return StartSchedulerImplementation(FALSE);
}

HX_RESULT    HXScheduler::StartSchedulerTimerFixup()
{
    return StartSchedulerImplementation(TRUE);
}

HX_RESULT    HXScheduler::StartSchedulerImplementation(HXBOOL TimerFixup)
{
    HX_RESULT theErr = HXR_OK;
    /* Stop any already running scheduler*/
    StopScheduler();
    (void) gettimeofday((Timeval*)&m_CurrentTimeVal, 0);
    m_ulLastUpdateTime = HX_GET_TICKCOUNT();

#if defined(_WIN32) || defined(THREADS_SUPPORTED)
    if (m_bIsInterruptEnabled)
    {
	if (!m_pAsyncTimer)
	{
	    m_pAsyncTimer = new CAsyncTimer(m_pContext, this);
	}

	if (!m_pAsyncTimer)
	{
	    return HXR_OUTOFMEMORY;
	}

	m_pAsyncTimer->SetGranularity(m_ulMinimumGranularity);
        theErr = m_pAsyncTimer->StartTimer();
    }
#endif // _WIN32 || THREADS_SUPPORTED

    if (!m_pTimeline)
    {
	m_pTimeline = new Timeline;
    }

    if (m_pTimeline)
    {
	m_pTimeline->Init( (IUnknown*) m_pContext, this, m_bUseDeferredTask);
#if defined(_MACINTOSH) && defined(THREADS_SUPPORTED)
	m_pTimeline->SetCoreMutex(m_pCoreMutex);
#endif
	m_pTimeline->SetStartTime(0);
	m_ulCurrentGranularity = m_ulMinimumGranularity;
	m_pTimeline->SetGranularity(m_ulCurrentGranularity);	
	if (TimerFixup)
	    m_pTimeline->SetTimerFixup(TRUE);
	m_pTimeline->Resume();
    }
    else
    {
	theErr = HXR_OUTOFMEMORY;
    }
    return theErr;
}

STDMETHODIMP
HXScheduler::StopScheduler()
{
#if defined(_WIN32) || defined(THREADS_SUPPORTED)
    if (m_pAsyncTimer)
    {
	m_pAsyncTimer->StopTimer();
	HX_DELETE(m_pAsyncTimer);
    }
#endif    
    if (m_pTimeline)
    {
	m_pTimeline->Pause();
	m_pTimeline->Done();
    }

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *		OnTimeSync
 *	Purpose:
 *		TBD
 *
 */
STDMETHODIMP
HXScheduler::OnTimeSync(HXBOOL bAtInterrupt)
{
    
    HX_RESULT theErr = HXR_OK;
    
    if (m_pCoreMutex)
    {
	m_pCoreMutex->Lock();
    }

    if (!m_bLocked)
    {
	m_bLocked = TRUE;
	theErr = ExecuteCurrentFunctions (bAtInterrupt);
	m_bLocked = FALSE;
    }

    if (m_pCoreMutex)
    {
	m_pCoreMutex->Unlock();
    }
    return theErr;
}

STDMETHODIMP_(HXBOOL)
HXScheduler::IsAtInterruptTime(void)
{
#ifdef _MACINTOSH
    return !IsMacInCooperativeThread();
#elif defined (_WIN32)  || defined(THREADS_SUPPORTED)
    return (m_bIsInterruptEnabled &&
	    m_pAsyncTimer->InTimerThread());
#endif
    return FALSE;
}

HXBOOL HXScheduler::UpdateCurrentTime(HXBOOL bAtInterrupt, 
				      HXBOOL& bShouldServiceSystem, 
                                      HXBOOL& bShouldServiceInterrupt,
				      HXBOOL& bShouldServiceInterruptOnly)
{
    HXBOOL    bResult = TRUE;

#if defined(_WINDOWS) || defined(_WIN32) || defined (_MACINTOSH) || defined(THREADS_SUPPORTED) || defined(_SYMBIAN)

    UINT32 ulCurrentTime = HX_GET_TICKCOUNT();
    UINT32 ulElapsedTime = CALCULATE_ELAPSED_TICKS(m_ulLastUpdateTime, ulCurrentTime);

//    UINT32 ulCurrentTimeBetter = HX_GET_BETTERTICKCOUNT();
//{FILE* f1 = ::fopen("d:\\temp\\better.txt", "a+"); ::fprintf(f1, "Diff %lu %lu %lu %lu\n", ulCurrentTime, ulCurrentTimeBetter, ulElapsedTime, CALCULATE_ELAPSED_TICKS(ulCurrentTimeBetter, ulCurrentTime));::fclose(f1);}

    if (ulElapsedTime >= m_ulInterruptNextDueTime)
    {
	bShouldServiceInterrupt = TRUE;
    }
    else
    {
        m_ulInterruptNextDueTime -= ulElapsedTime;
    }

    // If threads are not enabled, interrupt time execution is not available and thus
    // we consider execution of all scheduled tasks.
#if !defined(THREADS_SUPPORTED)
    HX_ASSERT(bAtInterrupt == FALSE);
    bAtInterrupt = FALSE;

    if (ulElapsedTime >= m_ulInterruptOnlyNextDueTime)
    {
	bShouldServiceInterruptOnly = TRUE;
    }
    else
    {
        m_ulInterruptOnlyNextDueTime -= ulElapsedTime;
    }
#else	// THREADS_SUPPORTED
    if (bAtInterrupt)
    {
	if (ulElapsedTime >= m_ulInterruptOnlyNextDueTime)
	{
	    bShouldServiceInterruptOnly = TRUE;
	}
	else
        {
            m_ulInterruptOnlyNextDueTime -= ulElapsedTime;
        }
    }
    else
#endif	// THREADS_SUPPORTED
    {
	if (ulElapsedTime >= m_ulSystemNextDueTime)
	{
	    bShouldServiceSystem = TRUE;
	}
	else
        {
            m_ulSystemNextDueTime -= ulElapsedTime;
        }
    }

//{FILE* f1 = ::fopen("d:\\temp\\pq.txt", "a+"); ::fprintf(f1, "\nUpdateCurrentTime %d %lu", m_ulCoreThreadID == GetCurrentThreadId(), CALCULATE_ELAPSED_TICKS(m_ulLastUpdateTime, ulCurrentTime));::fclose(f1);}
    m_CurrentTimeVal += (1000 * ulElapsedTime);
    m_ulLastUpdateTime  = ulCurrentTime;
#else
    // If threads are not enabled, interrupt time execution is not available and thus
    // we execute all scheduled tasks regardless.
    HX_ASSERT(bAtInterrupt == FALSE);

    bShouldServiceInterrupt = TRUE;
    bShouldServiceInterruptOnly = TRUE;
    bShouldServiceSystem = TRUE;

    (void) gettimeofday(&m_CurrentTimeVal, 0);
#endif

    if (!(bShouldServiceSystem || bShouldServiceInterrupt || bShouldServiceInterruptOnly))
    {
	bResult = FALSE;

	if (!m_pScheduler->empty() &&
	    m_pScheduler->head_time() != m_headTime)
	{
	    m_headTime = m_pScheduler->head_time();
	    bResult = TRUE;
	}
	    
	if (!m_pInterruptTimeScheduler->empty() &&
	    m_pInterruptTimeScheduler->head_time() != m_interruptHeadTime)
	{
	    m_interruptHeadTime = m_pInterruptTimeScheduler->head_time();
	    bResult = TRUE;
	}

	if (!m_pInterruptTimeOnlyScheduler->empty() &&
	    m_pInterruptTimeOnlyScheduler->head_time() != m_interruptOnlyHeadTime)
	{
	    m_interruptOnlyHeadTime = m_pInterruptTimeOnlyScheduler->head_time();
	    bResult = TRUE;
	}
    }
        
    return bResult;
}

STDMETHODIMP
HXScheduler::SetInterrupt(HXBOOL bInterruptenable)
{
    m_bIsInterruptEnabled = bInterruptenable;
    return HXR_OK;
}

STDMETHODIMP_(HXBOOL)
HXScheduler::IsInterruptEnabled(void)
{
#if defined(_WIN32) || defined(THREADS_SUPPORTED)
    return m_bIsInterruptEnabled;
#else
    return FALSE;
#endif
}

STDMETHODIMP
HXScheduler::SetMutex(IHXMutex* pMutex)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (!m_pCoreMutex)
    {
	m_pCoreMutex = pMutex;
	HX_ADDREF(m_pCoreMutex);

	retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP
HXScheduler::GetMutex(REF(IHXMutex*) pMutex)
{
    HX_RESULT	rc = HXR_OK;

    pMutex = NULL;

    if (m_pCoreMutex)
    {
	pMutex = m_pCoreMutex;
	HX_ADDREF(pMutex);
    }
    else
    {
	rc = HXR_FAILED;
    }

    return rc;
}

STDMETHODIMP
HXScheduler::NotifyPlayState(HXBOOL bInPlayingState)
{
#if defined(_WIN32) || defined(THREADS_SUPPORTED)
    if (m_pAsyncTimer)
    {
	m_pAsyncTimer->NotifyPlayState(bInPlayingState);
    }
#endif
    return HXR_OK;
}

void HXScheduler::GetTime(Timeval* pCurrentTimeVal)
{
#if defined(_WINDOWS) || defined(_WIN32) || defined (_MACINTOSH) || defined(THREADS_SUPPORTED) || defined(_SYMBIAN)

    UINT32 ulCurrentTime = HX_GET_TICKCOUNT();
    UINT32 ulElapsedTime = CALCULATE_ELAPSED_TICKS(m_ulLastUpdateTime, ulCurrentTime);

    *pCurrentTimeVal = m_CurrentTimeVal;
    *pCurrentTimeVal += (1000 * ulElapsedTime);

#else
    (void) gettimeofday(pCurrentTimeVal, 0);
#endif

    return;
}


UINT32 HXScheduler::GetGranularity()
{
    return m_ulCurrentGranularity;
}

STDMETHODIMP HXScheduler::PauseScheduler()
{
    if(m_pTimeline != NULL)
    {
        m_pTimeline->Pause();
    }
    return HXR_OK;
}

STDMETHODIMP HXScheduler::ResumeScheduler()
{
    if(m_pTimeline != NULL)
    {
        m_pTimeline->Resume();
    }
    return HXR_OK;
}

STDMETHODIMP_(HXBOOL) HXScheduler::IsPaused()
{
    if(m_pTimeline != NULL)
    {
        return m_pTimeline->IsPaused();
    }
	
    return FALSE;
}

HXSchedulerTimer::HXSchedulerTimer()
    : m_ulLastCallTime(0),
      m_ulTargetTime(0),
      m_ulSleepTime(0)
{}

HXSchedulerTimer::~HXSchedulerTimer()
{}


void HXSchedulerTimer::Init(UINT32 ulFreqInMS)
{
    HX_ASSERT(ulFreqInMS);

    m_ulTargetTime = ulFreqInMS;
    
    //Init our moving average. The size of the moving average should
    //not be too long as it will take longer to react to long
    //OnTimeSync calls. If it is too short then you will not get as
    //good of a true average number of calls to OnTimeSync, which
    //should be 1000/ulSleepTime calls-per-second. Another problem
    //with short moving averages is they will 'cool' down too soon if
    //we encounter bursty CPU usage. We also prime the moving average
    //with our ideal average.
    movAverage.Init(zm_nMovingAveSize);
    movAverage.AddSample(m_ulTargetTime);
}


ULONG32 HXSchedulerTimer::AddSample(UINT32 ulTS1, UINT32 ulTS2)
{
    if( 0 != m_ulLastCallTime )
    {
        movAverage.AddSample(ulTS2-m_ulLastCallTime);
            
        UINT32 ulMovAve = movAverage.Average();
        INT32  delta    = m_ulTargetTime-ulMovAve;
        INT32  time     = (INT32)m_ulTargetTime+(delta);
        UINT32 ulOTS    = ulTS2-ulTS1;
            
        //Never sleep longer then our m_ulTargetTime
        time = HX_MIN(time, m_ulTargetTime);

        //We can't sleep less then zero ms and we never sleep if the
        //last call to OnTimeSync took longer then our target time.
        if( time<0 || m_ulTargetTime<=ulOTS)
        {
            time = 0;
        }

        m_ulSleepTime = (UINT32)time * 1000;

        m_ulLastCallTime = ulTS2;
    }
    else
    {
        m_ulLastCallTime = ulTS2;
        movAverage.AddSample(ulTS2-ulTS1);
        
        //Never sleep until we get a good sample.
        m_ulSleepTime = 1;
    }

    return m_ulSleepTime;



    //XXXgfw Below is an alternate method for computing sleep time
    //from the samples. It is being left in place so that we can test
    //with it after several other changes get checked in.

//    INT32 lNextInterval;

//    // If there is no previous call, we'll just assume the previous
//    // call completed perfectly
//    if (m_ulPrevTS1 == 0)
//    {
//        m_ulPrevTS1 = ulTS1 - m_ulTargetTime;
//    }

//    m_lTimeDeficit += (INT32) (ulTS2 - m_ulPrevTS1 - m_ulTargetTime);
//    if (m_lTimeDeficit < 0)
//    {
//        // Do not allow time credit in order not to penalize the
//        // accuracy future timings
//        m_lTimeDeficit = 0;
//    }

//    // We need to make up the deficit
//    lNextInterval = m_ulTargetTime - m_lTimeDeficit;

//    //  0 delay is best caller can do
//    if (lNextInterval < 0)
//    {
//        lNextInterval = 0;
//    }

//    m_ulPrevTS1  = ulTS1;

//    return (UINT32) lNextInterval; 
    
        
}


UINT32 HXSchedulerTimer::GetSleepTime() const
{
    return m_ulSleepTime;
}

