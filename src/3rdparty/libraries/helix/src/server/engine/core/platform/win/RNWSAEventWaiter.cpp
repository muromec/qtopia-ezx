/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: RNWSAEventWaiter.cpp,v 1.4 2004/05/15 01:03:13 tmarshall Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
/*
 *  HXWSAEventWaiter.cpp
 *
 *  HXWSAEventWaiter is a class that can wait on more than WSA_MAXIMUM_WAIT_EVENTS
 *  WSAEvents.
 *
 */

#include "hlxclib/sys/socket.h"
#include "HXWSAEventWaiter.h"
#include "hxassert.h"

/*
 * HXWSAEventWaiter methods.
 */

HXWSAEventWaiter::HXWSAEventWaiter()
: m_hSignalAllThreads(0)
, m_dwNumThreads(0)
, m_pThreads(0)
, m_pThreadsAreDone(0)
, m_pHelperThreads(0)
, m_dwFairCounter(0)
{
}

HXWSAEventWaiter::~HXWSAEventWaiter()
{
    DWORD dw;
    for(dw = 0; dw < m_dwNumThreads; dw++)
    {
	m_pHelperThreads[dw]->m_bPleaseDieNow = TRUE;
	SetEvent(m_pHelperThreads[dw]->m_hGoAheadAndWait);
	CloseHandle(m_pThreads[dw]);
    }
    if(m_pThreads)
    {
	delete[] m_pThreads;
	m_pThreads = 0;
    }
    if(m_pHelperThreads)
    {
	delete[] m_pHelperThreads;
	m_pHelperThreads = 0;
    }
    if(m_pThreadsAreDone)
    {
	delete[] m_pThreadsAreDone;
	m_pThreadsAreDone = 0;
    }
    if(m_hSignalAllThreads)
    {
	WSACloseEvent(m_hSignalAllThreads);
	m_hSignalAllThreads = 0;
    }
}

DWORD
HXWSAEventWaiter::WaitForMassiveAmountsOfWSAEvents(
    DWORD dwNumEvents, WSAEVENT pEvents[], DWORD dwTimeout)
{
    /*
     * If the number of events is small enough, we just call
     * WSAWaitForMultipleEvents.
     */
    if(dwNumEvents <= MAX_WAIT_EVENTS)
    {
	return WSAWaitForMultipleEvents(
	    dwNumEvents, pEvents, FALSE, dwTimeout, FALSE);
    }

    /*
     * If we don't have enough threads, create enough.
     */
    if(!m_pThreads)
    {
	m_pThreads = new HANDLE[MAX_WAIT_EVENTS];
    }
    if(!m_pHelperThreads)
    {
	m_pHelperThreads = new HelperThread*[MAX_WAIT_EVENTS];
    }
    if(!m_pThreadsAreDone)
    {
	m_pThreadsAreDone = new HANDLE[MAX_WAIT_EVENTS];
    }
    if(!m_hSignalAllThreads)
    {
	m_hSignalAllThreads = WSACreateEvent();
    }

    /*
     * The -1 in the next line is because each thread also has to wait on
     * m_hSignalAllThreads.
     */
    DWORD dwNumThreadsNeeded = dwNumEvents / (MAX_WAIT_EVENTS-1);
    if(dwNumEvents % (MAX_WAIT_EVENTS-1))
    {
	dwNumThreadsNeeded ++;
    }
    while(m_dwNumThreads < dwNumThreadsNeeded)
    {
	DWORD dwId;
	m_pHelperThreads[m_dwNumThreads] = new HelperThread;
	m_pThreadsAreDone[m_dwNumThreads] = 
	    m_pHelperThreads[m_dwNumThreads]->m_hIAmDone;
	m_pHelperThreads[m_dwNumThreads]->m_pEvents[0] = 
	    m_hSignalAllThreads;
	m_pThreads[m_dwNumThreads] = CreateThread(
	    0, 0, (LPTHREAD_START_ROUTINE)StartRunningHere,
	    m_pHelperThreads[m_dwNumThreads], 0, &dwId);
	m_dwNumThreads++;
    }

    /*
     * Distrubute events to wait for accross our threads.
     */
    DWORD dw1, dw2;
    for(dw1 = 0; dw1 < dwNumThreadsNeeded; dw1++)
    {
	/*
	 * While we have not hit the limit for this particular thread
	 * and have not use up all of the events passed in ...
	 */
	for(dw2 = 0; dw2 < MAX_WAIT_EVENTS - 1 &&
	    dw1 * (MAX_WAIT_EVENTS-1) + dw2 < dwNumEvents; dw2++)
	{
	    m_pHelperThreads[dw1]->m_pEvents[dw2 + 1] = 
		pEvents[dw1 * (MAX_WAIT_EVENTS-1) + dw2];
	}
	m_pHelperThreads[dw1]->m_dwNumEvents = dw2 + 1;
	m_pHelperThreads[dw1]->m_dwTimeout = dwTimeout;
    }

    /*
     * Signal all threads to start waiting.  Use the m_dwFairCounter to ensure
     * that there is an even distrubution among the order of m_hGoAheadAndWait
     * signaling.  Without this, thread 1 one always get signaled first, and 
     * two second etc., so ones events would have a better chance of being taken
     * care of.
     */
    dw2 = m_dwFairCounter % dwNumThreadsNeeded;
    for(dw1 = 0; dw1 < dwNumThreadsNeeded; dw1++)
    {
	SetEvent(m_pHelperThreads[dw2]->m_hGoAheadAndWait);
	dw2 ++;
	if(dw2 == dwNumThreadsNeeded)
	{
	    dw2 = 0;
	}
    }
    m_dwFairCounter ++;

    /*
     * Wait for any thread to be done.
     */
    WaitForMultipleObjectsEx(dwNumThreadsNeeded,
	m_pThreadsAreDone, FALSE, INFINITE, FALSE);

    /*
     * Signal all threads to exit waiting.
     */
    WSASetEvent(m_hSignalAllThreads);

    /*
     * Wait for all threads to exit waiting.
     */
    WaitForMultipleObjectsEx(dwNumThreadsNeeded,
	m_pThreadsAreDone, TRUE, INFINITE, FALSE);

    /* 
     * Reset all threads IAmDone events.
     */
    for(dw1 = 0; dw1 < dwNumThreadsNeeded; dw1++)
    {
	ResetEvent(m_pHelperThreads[dw1]->m_hIAmDone);
    }

    /*
     * Reset all threads exit event.
     */
    WSAResetEvent(m_hSignalAllThreads);

    /*
     * Tally up results.
     */
    DWORD dwRetVal = WSA_WAIT_TIMEOUT;
    for(dw1 = 0; dw1 < dwNumThreadsNeeded; dw1++)
    {
	/*
	 * If this HelperThread exited because one of its events
	 * was signaled, we want to return that value, unless the 
	 * event that got signaled was my m_hSignalAllThreads.
	 */
	if(m_pHelperThreads[dw1]->m_dwRetVal ==
	    WSA_WAIT_EVENT_0)
	{
	    continue;
	}
	if(m_pHelperThreads[dw1]->m_dwRetVal > WSA_WAIT_EVENT_0 &&
	    m_pHelperThreads[dw1]->m_dwRetVal < WSA_WAIT_EVENT_0 +
	    m_pHelperThreads[dw1]->m_dwNumEvents)
	{
	    /*
	     * Okay, the event that got signaled was one that we care
	     * about.
	     */
	    dwRetVal = WSA_WAIT_EVENT_0 + 
		dw1 * (MAX_WAIT_EVENTS - 1) +
		m_pHelperThreads[dw1]->m_dwRetVal - WSA_WAIT_EVENT_0 - 1;
	    break;
	}
	if(m_pHelperThreads[dw1]->m_dwRetVal == WSA_WAIT_TIMEOUT)
	{
	    continue;
	}
	HX_ASSERT(0);
    }

    /*
     * Return results.
     */
    return dwRetVal;
}

/*
 * HXWSAEventWaiter::HelperThread methods.
 */
HXWSAEventWaiter::HelperThread::HelperThread()
: m_hGoAheadAndWait(0)
, m_hIAmDone(0)
, m_bPleaseDieNow(0)
{
    m_hGoAheadAndWait = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hIAmDone = CreateEvent(NULL, TRUE, FALSE, NULL);
}

HXWSAEventWaiter::HelperThread::~HelperThread()
{
    CloseHandle(m_hGoAheadAndWait);
    CloseHandle(m_hIAmDone);
}

DWORD
HXWSAEventWaiter::HelperThread::Live()
{
    while(1)
    {
	/*
	 * Wait until it is time to start waiting.
	 */
	WaitForSingleObjectEx(m_hGoAheadAndWait, INFINITE,
	    FALSE);

	/*
	 * Are we supposed to be exiting?
	 */
	if(m_bPleaseDieNow)
	{
	    return 0;
	}
	/*
	 * Wait on what we are supposed to wait on.
	 */
	m_dwRetVal =
	    WSAWaitForMultipleEvents(m_dwNumEvents,
	    m_pEvents, FALSE, m_dwTimeout, FALSE);

	/*
	 * Reset our state for waiting again.
	 */
	ResetEvent(m_hGoAheadAndWait);
	/*
	 * Raise our hand and say we are done.
	 */
	SetEvent(m_hIAmDone);

    }
    return 0;
}


DWORD
HXWSAEventWaiter::StartRunningHere(DWORD* pIt)
{
    HXWSAEventWaiter::HelperThread* pHelperThread = 
	(HXWSAEventWaiter::HelperThread*)pIt;

    return pHelperThread->Live();
}
