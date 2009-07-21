/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: carbthrd.h,v 1.8 2005/03/14 19:35:27 bobclark Exp $
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

#include "hxtypes.h" 
#include "hxslist.h"
#include "hxthread.h"
#ifndef _MAC_MACHO
#include <Multiprocessing.h>
#endif

#ifndef _HX_CARBON_THREAD
#define _HX_CARBON_THREAD

class HXCarbonThread : public HXThread
{
public:
    static ULONG32     GetCurrentThreadID();
    			HXCarbonThread	(void);
    virtual		~HXCarbonThread	(void);

    virtual HX_RESULT	CreateThread
					(void* (pExecAddr(void*)), 
					 void* pArg,
					 ULONG32 ulCreationFlags = 0);

    virtual HX_RESULT	Suspend		(void);
			    
    virtual HX_RESULT	Resume		(void);

    virtual HX_RESULT	SetPriority	(UINT32 ulPriority);

    virtual HX_RESULT	GetPriority	(UINT32& ulPriority);

    virtual HX_RESULT	YieldTimeSlice	(void);

    virtual HX_RESULT	Exit		(UINT32 ulExitCode);
    
    virtual HX_RESULT	GetThreadId	(UINT32& ulThreadId);
    
    virtual HX_RESULT	PostMessage(HXThreadMessage* pMsg, void* pWindowHandle = 0);

    virtual HX_RESULT	GetMessage(HXThreadMessage* pMsg, UINT32 ulMsgFilterMix = 0, UINT32 ulMsgFilterMax = 0);

    virtual HX_RESULT	PeekMessage(HXThreadMessage* pMsg, UINT32 ulMsgFilterMix = 0, UINT32 ulMsgFilterMax = 0, HXBOOL bRemoveMessage = TRUE);
    
    virtual HX_RESULT	PeekMessageMatching(HXThreadMessage* pMsg, HXThreadMessage* pMatch, HXBOOL bRemoveMessage);

    virtual HX_RESULT	DispatchMessage(HXThreadMessage* pMsg);

private:

    HXMutex*	m_pQueueMutex;
    MPSemaphoreID	m_pQueuePostSemaphore;
    MPTaskID	m_mpTaskID;
    MPQueueID	m_mpQueueID;
    MPQueueID	m_mpInternalTerminationNotificationQueueID;
};

class HXCarbonEvent : public HXEvent
{
public:
			HXCarbonEvent	(const char* pEventName = NULL, HXBOOL bManualReset = TRUE);
    virtual		~HXCarbonEvent	(void);

    virtual HX_RESULT	SignalEvent	(void);

    virtual HX_RESULT	ResetEvent	(void);

    virtual void*	GetEventHandle	(void);

    virtual HX_RESULT	Wait		(UINT32 uTimeoutPeriod = ALLFS);
    
private:

    MPSemaphoreID	m_mpSemaphoreID;
    HXBOOL		m_IsManuallyReset;
};

class HXCarbonManualEvent : public HXEvent
{
public:

			HXCarbonManualEvent(const char* pEventName = NULL);
    virtual		~HXCarbonManualEvent(void);
    
    virtual HX_RESULT	SignalEvent	(void);
    virtual HX_RESULT	ResetEvent	(void);
    virtual void*	GetEventHandle	(void);
    virtual HX_RESULT	Wait		(UINT32 uTimeoutPeriod = ALLFS);
    
private:

    HXMutex*		m_pMutex;
    HXBOOL		m_bIsSignalled;
    MPSemaphoreID	m_InternalSemaphoreID;
};

class HXCarbonMutex : public HXMutex
{
public:
    HXCarbonMutex	(void);
    ~HXCarbonMutex	(void);

    virtual HX_RESULT	Lock		(void);

    virtual HX_RESULT   Unlock		(void);

    virtual HX_RESULT   Trylock		(void);
    
private:

    MPCriticalRegionID mCriticalRegion;

};

class HXCarbonSemaphore
{
public:
    
    HXCarbonSemaphore(UINT32 unInitialCount=0);
    ~HXCarbonSemaphore();

    virtual HX_RESULT Post();
    virtual HX_RESULT Wait();
    virtual HX_RESULT TryWait();
    
private:
    MPSemaphoreID	m_mpSemaphoreID;   
};

#if 0

class HXCarbonAsyncTimer
{
public:

    // SetTimer and KillTimer emulate Windows' ::SetTimer and ::KillTimer
    // functions.
    
    static UINT32	SetTimer(ULONG32 ulTimeOut, HXThread* pReceivingThread);
    static HXBOOL		KillTimer(UINT32 ulTimerID);

protected:

		HXCarbonAsyncTimer(ULONG32 ulTimeOut, HXThread* pReceivingThread);
		~HXCarbonAsyncTimer();
		
    static void	MyCarbonTimer(EventLoopTimerRef, HXCarbonAsyncTimer* pAsyncTimer);

    HXThread*	m_pReceivingThread;
    ULONG32	m_ulTimeout;
    EventLoopTimerUPP	m_CarbonTimerUPP;
    EventLoopTimerRef	m_CarbonTimerRef;
    HXThreadMessage*	m_msg;
};

#endif

#endif /*_HX_CARBON_THREAD*/