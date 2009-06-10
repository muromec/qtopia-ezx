/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxoptsc.cpp,v 1.17 2008/09/07 11:07:07 pbasic Exp $
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

#ifdef _WINDOWS
#include "hlxclib/windows.h"
#endif

#include "timeval.h"
#include "clientpq.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "timeline.h"
#include "hxtick.h"
#include "pckunpck.h"
#include "hxthread.h"
#include "hxoptsc.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define MINIMUM_GRANULARITY   10
#define MINIMUM_DIFFERENCE    1
#define	ALLFS                 0xFFFFFFFF

void* ThreadRoutine(void * pArg);

// HXOptimizedScheduler...
HXOptimizedScheduler::HXOptimizedScheduler(IUnknown* pContext) :
     m_lRefCount (0)
    ,m_pPQ(0)
    ,m_pID(NULL)
    ,m_pPQMutex(NULL)
    ,m_pContext(pContext)
    ,m_pScheduler(NULL)
    ,m_ulLastUpdateTime(0)
    ,m_ulLastSyncTime(0)
    ,m_pMutex(NULL)
    ,m_pThread(NULL)
    ,m_pQuitEvent(NULL)
    ,m_pSleepEvent(NULL)
    ,m_ulCurrentGranularity(MINIMUM_GRANULARITY)
    ,m_bIsDone(FALSE)
{
    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pPQMutex, m_pContext);

    m_pID    = new CHXID(50);
    m_pPQ    = new ClientPQ(pContext, m_pID, m_pPQMutex);

    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);  

    gettimeofday((Timeval*)&m_CurrentTimeVal, 0);
    m_ulLastSyncTime = m_ulLastUpdateTime = HX_GET_BETTERTICKCOUNT();

    if (m_pContext)
    {
	m_pContext->AddRef();
    }
}

HXOptimizedScheduler::~HXOptimizedScheduler()
{
    StopScheduler();

    HX_DELETE(m_pPQ);
    HX_DELETE(m_pID);
    HX_RELEASE(m_pPQMutex);
    HX_RELEASE(m_pMutex);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pScheduler);
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
STDMETHODIMP HXOptimizedScheduler::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXOptimizedScheduler), (IHXOptimizedScheduler*)this },
            { GET_IIDHANDLE(IID_IHXOptimizedScheduler2),(IHXOptimizedScheduler2*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXOptimizedScheduler*)this },
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
STDMETHODIMP_(ULONG32) HXOptimizedScheduler::AddRef()
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
STDMETHODIMP_(ULONG32) HXOptimizedScheduler::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}


/*
 * HXOptimizedScheduler methods
 */
/************************************************************************
 *	Method:
 *		IHXOptimizedScheduler::Enter
 *	Purpose:
 *		enter objects in the service queue
 */
STDMETHODIMP_(CallbackHandle)
HXOptimizedScheduler::RelativeEnter(IHXCallback* pCallback, ULONG32 ulTime)
{
    /*
     * A RelativeEnter() of 0 ms is a special case that needs to be
     * AbsoluteEnter() of 0
     */
    if (ulTime == 0)
    {
	HXTimeval rVal;

	rVal.tv_sec = rVal.tv_usec = 0;
	return AbsoluteEnter(pCallback, rVal);
    }

    if (m_pScheduler)
    {
	return m_pScheduler->RelativeEnter(pCallback, ulTime);
    }

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

    lTime.tv_sec    = secs;
    lTime.tv_usec   = usecs;

    Timeval now;
    now.tv_sec = m_CurrentTimeVal.tv_sec;
    now.tv_usec = m_CurrentTimeVal.tv_usec;
    now += lTime;
    
    m_pMutex->Lock();
    CallbackHandle handle = m_pPQ->enter(now, pCallback);
    UINT32 ulNumMs = 0;
    if (GetNextEventDueTime(ulNumMs))
    {
	m_ulCurrentGranularity = ulNumMs;
    }
    else
    {
	m_ulCurrentGranularity = 0xFFFFFFFF;
    }

    m_pSleepEvent->SignalEvent();
    m_pMutex->Unlock();
    return handle;
}

/************************************************************************
 *	Method:
 *	    IHXOptimizedScheduler::AbsoluteEnter
 *	Purpose:
 *	    enter objects in the service queue at absolute time
 */
STDMETHODIMP_(CallbackHandle)
HXOptimizedScheduler::AbsoluteEnter(IHXCallback* pCallback, HXTimeval tVal)
{
    if (m_pScheduler)
    {
	return m_pScheduler->AbsoluteEnter(pCallback, tVal);
    }

    Timeval lTime;

    lTime.tv_sec    = tVal.tv_sec;
    lTime.tv_usec   = tVal.tv_usec;

    m_pMutex->Lock();
    CallbackHandle handle = m_pPQ->enter(lTime, pCallback);
    UINT32 ulNumMs = 0;
    if (GetNextEventDueTime(ulNumMs))
    {
	m_ulCurrentGranularity = ulNumMs;
    }
    else
    {
	m_ulCurrentGranularity = 0xFFFFFFFF;
    }
    m_pSleepEvent->SignalEvent();
    m_pMutex->Unlock();
    return handle;
}

/************************************************************************
 *	Method:
 *		IHXOptimizedScheduler::Remove
 *	Purpose:
 *		remove objects from the service queue
 */
STDMETHODIMP HXOptimizedScheduler::Remove(CallbackHandle Handle)
{
    if (m_pScheduler)
    {
	return m_pScheduler->Remove(Handle);
    }

    m_pMutex->Lock();
    m_pPQ->remove(Handle);
    m_pMutex->Unlock();
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXOptimizedScheduler::GetCurrentSchedulerTime
 *	Purpose:
 *	    gives the current time in the timeline of the scheduler...
 */
STDMETHODIMP_(HXTimeval) HXOptimizedScheduler::GetCurrentSchedulerTime(void)
{
    if (m_pScheduler)
    {
	return m_pScheduler->GetCurrentSchedulerTime();
    }

    return m_CurrentTimeVal;
}

HX_RESULT HXOptimizedScheduler::ExecuteCurrentFunctions(void)
{
    Timeval now;

    UpdateCurrentTime(&now);

    m_pPQ->execute(now);
    
    m_pMutex->Lock();
    /*
     * Don't execute more then 100 immediate elements.  We don't wanna
     * hold on too long and spin here.
     */
    int count = 0;
    // Keep executing until there are no more zero time elements
    while (m_pPQ->immediate() && count < 100)
    {
        m_pMutex->Unlock();
	count += m_pPQ->execute(now);
        m_pMutex->Lock();
    }

    UINT32 ulNumMs = 0;
    if (GetNextEventDueTime(ulNumMs))
    {
	m_ulCurrentGranularity = ulNumMs;
    }
    else
    {
	m_ulCurrentGranularity = 0xFFFFFFFF;
    }

    m_pMutex->Unlock();

    return HXR_OK;
}

HXBOOL	
HXOptimizedScheduler::GetNextEventDueTime(UINT32& ulNumMs)
{
    if (m_pPQ->empty())
    {
	return FALSE;
    }
    else
    {
	Timeval now;
	now.tv_sec = m_CurrentTimeVal.tv_sec;
	now.tv_usec = m_CurrentTimeVal.tv_usec;

	Timeval timeout	  = m_pPQ->head_time();
	if (timeout > now)
	{
	    timeout = timeout - now;
	    ulNumMs = timeout.tv_sec * 1000 + timeout.tv_usec / 1000;
	}
	else
	{
	    ulNumMs = 0;
	}
	return TRUE;
    }
}

STDMETHODIMP
HXOptimizedScheduler::StartScheduler()
{
#ifndef THREADS_SUPPORTED
    if (!m_pScheduler)
    {
	m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
    }
#endif

    if (m_pScheduler)
    {
	return HXR_OK;
    }

    HX_RESULT theErr = HXR_OK;
    /* Stop any already running scheduler*/
    StopScheduler();
    gettimeofday((Timeval*)&m_CurrentTimeVal, 0);
    m_ulLastSyncTime = m_ulLastUpdateTime = HX_GET_BETTERTICKCOUNT();

    CreateEventCCF((void**)&m_pQuitEvent, m_pContext, NULL, TRUE);
    CreateEventCCF((void**)&m_pSleepEvent, m_pContext, NULL, FALSE);
    m_bIsDone = FALSE;

    theErr = CreateInstanceCCF(CLSID_IHXThread, (void**)&m_pThread, m_pContext);
    if (!theErr)
    {
	theErr = m_pThread->CreateThread(ThreadRoutine, (void*) this, 0);
    }

    if (!theErr)
    {
	m_pThread->SetThreadName("Optimized Scheduler Thread");
#ifdef _WIN32
	/* We should abstract priority level in thread class */ 
	theErr = m_pThread->SetPriority(THREAD_PRIORITY_HIGHEST);//THREAD_PRIORITY_TIME_CRITICAL);
#endif /*_WIN32*/
    }

    return theErr;
}

STDMETHODIMP
HXOptimizedScheduler::StopScheduler()
{
    if (m_pScheduler)
    {	
	return HXR_OK;
    }

    if (m_pThread)
    {
	m_bIsDone = TRUE;
	m_pSleepEvent->SignalEvent();
	m_pQuitEvent->Wait(ALLFS);
	m_pThread->Exit(0);
	HX_RELEASE(m_pThread);
    }

    HX_RELEASE(m_pQuitEvent);
    HX_RELEASE(m_pSleepEvent);

    return HXR_OK;
}

void
HXOptimizedScheduler::UpdateCurrentTime(Timeval* pNow)
{
#if defined(_WINDOWS) || defined(_WIN32) || defined(_UNIX)

#define MINIMUM_SYNC_TIME   5000

    UINT32 ulCurrentTime = HX_GET_BETTERTICKCOUNT();

    if (CALCULATE_ELAPSED_TICKS(m_ulLastSyncTime, ulCurrentTime) > MINIMUM_SYNC_TIME)
    {
	gettimeofday(pNow, 0);

	m_CurrentTimeVal.tv_sec = pNow->tv_sec;
	m_CurrentTimeVal.tv_usec = pNow->tv_usec;	
	m_ulLastSyncTime = m_ulLastUpdateTime = ulCurrentTime;
	return;
    }

    UINT32 ulElapsedTime = 1000 * CALCULATE_ELAPSED_TICKS(m_ulLastUpdateTime, ulCurrentTime);

    pNow->tv_sec    = m_CurrentTimeVal.tv_sec;
    pNow->tv_usec   = m_CurrentTimeVal.tv_usec;	
    (*pNow) += ulElapsedTime;

    m_CurrentTimeVal.tv_sec = pNow->tv_sec;
    m_CurrentTimeVal.tv_usec = pNow->tv_usec;	

    m_ulLastUpdateTime = ulCurrentTime;
#else
    gettimeofday(pNow, 0);
    m_CurrentTimeVal.tv_sec = pNow->tv_sec;
    m_CurrentTimeVal.tv_usec = pNow->tv_usec;	
#endif
}

void* ThreadRoutine(void * pArg)
{
    HXOptimizedScheduler* pOptimizedScheduler = 
			    (HXOptimizedScheduler*) pArg;

    IHXEvent*	pSleepEvent = pOptimizedScheduler->m_pSleepEvent;
    

    while (!pOptimizedScheduler->m_bIsDone)
    {
	pOptimizedScheduler->ExecuteCurrentFunctions();
        UINT32 gran = pOptimizedScheduler->m_ulCurrentGranularity;
	pSleepEvent->Wait(gran);
    }

    pOptimizedScheduler->m_pQuitEvent->SignalEvent();

    return (void*) 0;
}

