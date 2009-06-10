/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: RNWSAEventWaiter.h,v 1.4 2004/05/15 01:03:13 tmarshall Exp $ 
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
 *  HXWSAEventWaiter.h
 *  
 *  HXWSAEventWaiter is a class that can wait on more than WSA_MAXIMUM_WAIT_EVENTS
 *  WSAEvents.
 *
 */

#ifndef _HXWSAEVENTWAITER_H
#define _HXWSAEVENTWAITER_H

enum
{
    MAX_WAIT_EVENTS = WSA_MAXIMUM_WAIT_EVENTS
};

class HXWSAEventWaiter
{
public:
    HXWSAEventWaiter();
    ~HXWSAEventWaiter();

    DWORD WaitForMassiveAmountsOfWSAEvents(
	DWORD dwNumEvents, WSAEVENT pEvents[], DWORD dwTimeout);

private:
    class HelperThread
    {
	HelperThread();
	~HelperThread();
	HANDLE m_hGoAheadAndWait;
	HANDLE m_hIAmDone;
	WSAEVENT m_pEvents[MAX_WAIT_EVENTS];
	DWORD m_dwNumEvents;
	DWORD m_dwTimeout;
	DWORD Live();
	DWORD m_dwRetVal;
	BOOL  m_bPleaseDieNow;
	friend HXWSAEventWaiter;
    };

    static DWORD StartRunningHere(DWORD *);

    HANDLE* m_pThreadsAreDone; 
    HANDLE* m_pThreads;
    HelperThread** m_pHelperThreads;
    DWORD m_dwNumThreads;
    WSAEVENT m_hSignalAllThreads;
    DWORD m_dwFairCounter;

};

#endif
