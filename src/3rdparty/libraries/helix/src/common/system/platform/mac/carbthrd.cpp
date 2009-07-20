/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: carbthrd.cpp,v 1.9 2007/07/06 20:41:56 jfinnecy Exp $
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

#if defined(_CARBON) || defined(_MAC_UNIX)

#include "platform/mac/carbthrd.h"
#include "hxmsgs.h" //for HXMSG_ASYNC_TIMER message.

HXCarbonThread::HXCarbonThread()
  : m_mpTaskID(NULL)
  , m_mpQueueID(NULL)
  , m_pQueueMutex(NULL)
  , m_pQueuePostSemaphore(kInvalidID)
  , m_mpInternalTerminationNotificationQueueID(NULL)
{
    // xxxbobclark there's a kludgey main app thread wrapper thingie
    // for handling networked threading. It's a case where an
    // HXCarbonThread gets used without HXCarbonThread::CreateThread()
    // being called... but it requires m_mpQueueID. So we create it in
    // the ctor.
    HXMutex::MakeMutex(m_pQueueMutex);
    ::MPCreateSemaphore(INT_MAX, 0, &m_pQueuePostSemaphore);
    OSStatus osResult = MPCreateQueue(&m_mpQueueID);
}

HXCarbonThread::~HXCarbonThread()
{
    this->Exit(0);
    
    // xxxbobclark should I remove the queue here if it exists?
    HX_ASSERT(m_mpQueueID == NULL);
    HX_ASSERT(m_mpTaskID == NULL);
    HX_ASSERT(m_pQueueMutex == NULL);
    HX_ASSERT(m_pQueuePostSemaphore == kInvalidID);
}

HX_RESULT
HXCarbonThread::CreateThread( void* (pExecAddr(void*)), void* pArg, ULONG32 ulCreationFlags )
{
    HX_ASSERT(m_mpTaskID == NULL);
    
    HX_RESULT retval = HXR_OK;
    
    MPTaskOptions options = 0;
    
    if (ulCreationFlags & HX_CREATE_SUSPENDED)
    {
	options |= 1;
    }
    
    OSStatus osResult = MPCreateQueue(&m_mpInternalTerminationNotificationQueueID);
    
    if (osResult == noErr)
    {
	osResult = MPCreateTask((TaskProc)pExecAddr, pArg, 0, m_mpInternalTerminationNotificationQueueID, NULL, NULL, options, &m_mpTaskID);
    }
    
    if (osResult != noErr) retval = HXR_FAIL;
    
    return retval;
}

HX_RESULT
HXCarbonThread::Suspend(void)
{
    HX_RESULT retval = HXR_NOTIMPL;
    
    // xxxbobclark this is actually why interrupt-like callbacks are impossible
    // to simulate correctly under OS X: there's no suspend or resume available
    // in the MPTask API. I may need to grab the corresponding pthread and tell
    // it to suspend if the pthread API supports it.
    HX_ASSERT(!"Unimplemented Suspend!");
    return retval;
}

HX_RESULT
HXCarbonThread::Resume(void)
{
    HX_RESULT retval = HXR_NOTIMPL;
    HX_ASSERT(!"Unimplemented Resume!");
    return retval;
}

HX_RESULT
HXCarbonThread::SetPriority(UINT32 ulPriority)
{
    HX_RESULT retval = HXR_OK;
    
    OSStatus osStatus = MPSetTaskWeight(m_mpTaskID, ulPriority);
    
    return retval;
}

HX_RESULT
HXCarbonThread::GetPriority(UINT32& ulPriority)
{
    HX_RESULT retval = HXR_NOTIMPL;
    HX_ASSERT(!"Unimplemented GetPriority!");
    return retval;
}

HX_RESULT
HXCarbonThread::YieldTimeSlice(void)
{
    // unnecessary with MPTask implementation on Carbon.
    
    // I'll call MPYield anyway, although the MP API SDK
    // says "In most cases you should not need to call
    // this function".
    
    HX_RESULT retval = HXR_OK;
    MPYield();
    return retval;
}

HX_RESULT
HXCarbonThread::Exit(UINT32 ulExitCode)
{
    // xxxbobclark MPExit must be called from within the 
    // MPTask. That's why I think we need to use MPTerminateTask here.
    
    // This is also called by the destructor, just in case
    
    HX_RESULT retval = HXR_OK;
    
    OSStatus osStatus = noErr;
    if (m_mpTaskID && (::MPCurrentTaskID() != m_mpTaskID))
    {
	    // xxxbobclark The windows implementation of HXThread doesn't
	    // shoot the thread in the head, like ::MPTerminateTask() does...
	    // it just waits for it to end.
	    
	    // There is conflicting advice in Apple documentation on how advisable
	    // it is to call MPTerminateTask. Listing 3-3 Terminating Tasks, at
	    // http://developer.apple.com/techpubs/macosx/Carbon/oss/MultiPServices/Multitasking_MultiproServ/Concepts/MP.1b.html
	    // is pretty clear: "When you want to terminate a task, you should call
	    // the function MPTerminateTask"... but if you follow the link to
	    // MPTerminateTask, it says "you should be very careful when calling
	    // MPTerminateTask".
	    
	    // osStatus = ::MPTerminateTask(m_mpTaskID, NULL);

	    ::MPWaitOnQueue(m_mpInternalTerminationNotificationQueueID, NULL, NULL, NULL, kDurationForever);
	    ::MPDeleteQueue(m_mpInternalTerminationNotificationQueueID);
	    m_mpInternalTerminationNotificationQueueID = NULL;

	    m_mpTaskID = NULL;
    }
    
    if (m_mpQueueID)
    {
	::MPDeleteQueue(m_mpQueueID);
	m_mpQueueID = NULL;
    }
    
    HX_DELETE(m_pQueueMutex);
    
    if (m_pQueuePostSemaphore != kInvalidID)
    {
	::MPDeleteSemaphore(m_pQueuePostSemaphore);
	m_pQueuePostSemaphore = kInvalidID;
    }
    
    if (osStatus != noErr) retval = HXR_FAIL;
    
    return retval;
}


HX_RESULT
HXCarbonThread::GetThreadId(UINT32& ulThreadId)
{
    HX_RESULT retval = HXR_OK;
    
    ulThreadId = (UINT32) m_mpTaskID;
    return retval;
}

ULONG32
HXCarbonThread::GetCurrentThreadID()
{
    MPTaskID id = ::MPCurrentTaskID();
    return (ULONG32) id;
}

HX_RESULT
HXCarbonThread::PostMessage(HXThreadMessage* pMsg, void* pWindowHandle)
{
    HX_RESULT retval = HXR_OK;
    
    HX_ASSERT(pMsg);
    if (!pMsg) return HXR_FAIL;
    
    HX_ASSERT(m_mpQueueID != NULL);
    
    m_pQueueMutex->Lock();
    OSStatus osStatus = ::MPNotifyQueue(m_mpQueueID, (void*)pMsg->m_ulMessage,
			(void*)pMsg->m_pParam1, (void*)pMsg->m_pParam2);
    ::MPSignalSemaphore(m_pQueuePostSemaphore);
    m_pQueueMutex->Unlock();
    
    if (osStatus != noErr)
    {
	retval = HXR_FAIL;
    }
    
    return retval;
}

HX_RESULT
HXCarbonThread::GetMessage(HXThreadMessage* pMsg, 
			   UINT32 ulMsgFilterMix, 
			   UINT32 ulMsgFilterMax)
{
    HX_RESULT retval = HXR_OK;
    
    HX_ASSERT(m_mpQueueID != NULL);
    HX_ASSERT(pMsg);
    if (!pMsg) return HXR_FAIL;
    
    void* param1;
    void* param2;
    void* param3;
    
    OSStatus osStatus = ::MPWaitOnSemaphore(m_pQueuePostSemaphore, kDurationForever);
    
    if (osStatus == noErr)
    {
	m_pQueueMutex->Lock();
	osStatus = ::MPWaitOnQueue(m_mpQueueID, &param1, &param2, &param3, kDurationImmediate);
	HX_ASSERT(osStatus == noErr);
	m_pQueueMutex->Unlock();
    }
    
    if (osStatus == noErr)
    {
	pMsg->m_ulMessage = (UINT32)param1;
	pMsg->m_pParam1 = param2; // xxxbobclark I know, I know, this is crazy.
	pMsg->m_pParam2 = param3;
    }
    else
    {
	retval = HXR_FAIL;
    }
    
    return retval;
}

HX_RESULT
HXCarbonThread::PeekMessage(HXThreadMessage* pMsg, UINT32 ulMsgFilterMix, UINT32 ulMsgFilterMax, HXBOOL bRemoveMessage)
{
    // xxxbobclark OK so PeekMessage is going to grab the first message (if it exists)
    // and dupe it and replace it in the queue. It will eventually be more complicated
    // than this I think. I think I'll need to mutex-protect this routine and do some
    // wacky stuff to ensure that the queue is in the same order when it exits as it
    // is when it enters.
    
    HX_ASSERT(m_mpQueueID != NULL);
    HX_RESULT retval = HXR_OK;
    
    HX_ASSERT(pMsg);
    if (!pMsg) return HXR_FAIL;
    
    void* param1;
    void* param2;
    void* param3;
    
    m_pQueueMutex->Lock();
    OSStatus osStatus = ::MPWaitOnQueue(m_mpQueueID, &param1, &param2, &param3, kDurationImmediate);
    m_pQueueMutex->Unlock();
    
    if (osStatus == noErr)
    {
	pMsg->m_ulMessage = (UINT32)param1;
	pMsg->m_pParam1 = param2;
	pMsg->m_pParam2 = param3;
	
	if (!bRemoveMessage)
	{
	    // replace it in the queue. !!!xxxbobclark but this is
	    // in the wrong place!!!
	    HX_ASSERT(!"THIS IS NOT FINISHED! THE QUEUE IS BEING RESTORED INCORRECTLY!");
	    PostMessage(pMsg, NULL);
	}
	
    }
    else
    {
	pMsg->m_ulMessage = 0;
	retval = HXR_FAIL; // no event to get!
    }
    
    return retval;
}

HX_RESULT
HXCarbonThread::PeekMessageMatching( HXThreadMessage* pMsg,
                                      HXThreadMessage* pMatch,
                                      HXBOOL bRemoveMessage )
{
    // we're going to create a temp queue, copy each message from the main queue into it until we find
    // a matching message, then copy the rest
	
    HX_ASSERT(m_mpQueueID != kInvalidID);
    HX_RESULT retval = HXR_OK;
    
    HX_ASSERT(pMsg);
    if (!pMsg) return HXR_FAIL;
    
    HX_ASSERT(pMatch);
    if (!pMatch) return HXR_FAIL;
    
    void* param1;
    void* param2;
    void* param3;
    
    OSStatus osResult;
    
    m_pQueueMutex->Lock();

    // create a temp queue, all messages will be copied into this queue 
    // (except matching message if bRemoveMessage == TRUE)
    MPQueueID tempQ;
    osResult = ::MPCreateQueue(&tempQ);
    HX_ASSERT(tempQ != kInvalidID);

    HXBOOL foundAMatch = FALSE;
    
    const HXBOOL bSkipMessage  = (pMatch->m_ulMessage == 0);
    const HXBOOL bSkipParam1   = (pMatch->m_pParam1 == NULL);
    const HXBOOL bSkipParam2   = (pMatch->m_pParam2 == NULL);
    while((osResult = ::MPWaitOnQueue(m_mpQueueID, &param1, &param2, &param3, kDurationImmediate)) == noErr)
    {        
        // does it match?
        if( ( bSkipMessage || pMatch->m_ulMessage == (UINT32) param1 ) &&
            ( bSkipParam1  || pMatch->m_pParam1 == param2 ) &&
            ( bSkipParam2  || pMatch->m_pParam2 == param3 ) )
        {
            foundAMatch = TRUE;
            break;
        }
        
        // copy it into the temp queue
        ::MPNotifyQueue(tempQ, param1, param2, param3);
    }
    
    if( foundAMatch )
    {
	// copy the found message to the out variable
	pMsg->m_ulMessage = (UINT32)param1;
	pMsg->m_pParam1 = param2;
	pMsg->m_pParam2 = param3;
		
	// if we aren't supposed to remove it, re-post it
	if( !bRemoveMessage )
	{
	    ::MPNotifyQueue(tempQ, param1, param2, param3);
	}
		
	// now copy any remaining messages
        while((osResult = ::MPWaitOnQueue(m_mpQueueID, &param1, &param2, &param3, kDurationImmediate)) == noErr)
        {
            ::MPNotifyQueue(tempQ, param1, param2, param3);
        }
        
        retval = HXR_OK;
    }
    else
    {
        pMsg->m_ulMessage = 0;
        retval = HXR_FAIL;
    }
    
    // now that the original queue is empty, throw it away and keep the copy that we made
    ::MPDeleteQueue(m_mpQueueID);
    m_mpQueueID = tempQ;

    m_pQueueMutex->Unlock();

    return retval;
}

HX_RESULT
HXCarbonThread::DispatchMessage(HXThreadMessage* pMsg)
{
    HX_RESULT retval = HXR_NOTIMPL;
    HX_ASSERT(!"Unimplemented DispatchMessage!");
    return retval;
}



// HXCarbonEvent

HXCarbonEvent::HXCarbonEvent(const char* pEventName, HXBOOL bManualReset)
  : m_mpSemaphoreID(NULL)
  , m_IsManuallyReset( bManualReset )
{
    MPSemaphoreCount semaphoreMax = bManualReset ? INT_MAX : 1;
    OSStatus osStatus = ::MPCreateSemaphore(semaphoreMax, 0, &m_mpSemaphoreID);
    HX_ASSERT(osStatus == noErr);
}

HXCarbonEvent::~HXCarbonEvent()
{
    OSStatus osStatus = ::MPDeleteSemaphore(m_mpSemaphoreID);
    m_mpSemaphoreID = NULL;
    HX_ASSERT(osStatus == noErr);
}

HX_RESULT
HXCarbonEvent::SignalEvent(void)
{
    HX_RESULT retval = HXR_OK;
    OSStatus osStatus = ::MPSignalSemaphore(m_mpSemaphoreID);
    if (osStatus != noErr && osStatus != kMPInsufficientResourcesErr) retval = HXR_FAIL;
    
    return retval;
}

HX_RESULT
HXCarbonEvent::ResetEvent(void)
{
    HX_RESULT retval = HXR_OK;
    OSStatus osStatus;
    
    while(noErr == (osStatus = ::MPWaitOnSemaphore(m_mpSemaphoreID, kDurationImmediate)))
    {
    }
    
    return retval;
}

void*
HXCarbonEvent::GetEventHandle(void)
{
    return (void*)m_mpSemaphoreID;
}

HX_RESULT
HXCarbonEvent::Wait(UINT32 uTimeoutPeriod)
{
    HX_RESULT retval = HXR_OK;
    
    Duration timeout = ( uTimeoutPeriod == ALLFS ) ? kDurationForever : ( uTimeoutPeriod * kDurationMillisecond );
    
    OSStatus osStatus = ::MPWaitOnSemaphore(m_mpSemaphoreID, timeout);
    
    switch (osStatus)
    {
	case noErr:
	    if( m_IsManuallyReset )
	    {
	    	// needs to go back into the raised state until Reset is explicitly called
	    	::MPSignalSemaphore(m_mpSemaphoreID);
	    }
	    retval = HXR_OK;
	    break;
	
	case kMPTimeoutErr:
	    retval = HXR_WOULD_BLOCK; // winthrd.cpp returns this if the wait times out
	    break;
	
	default:
	    retval = HXR_FAIL;
	    break;
    }
    
    return retval;
}

// HXCarbonManualEvent

HXCarbonManualEvent::HXCarbonManualEvent(const char* pEventName)
 : m_pMutex(NULL)
 , m_bIsSignalled(FALSE)
 , m_InternalSemaphoreID(NULL)
{
    HXMutex::MakeMutex(m_pMutex);
    OSStatus osStatus = ::MPCreateSemaphore(1, 0, &m_InternalSemaphoreID);
}

HXCarbonManualEvent::~HXCarbonManualEvent()
{
    OSStatus osStatus = ::MPDeleteSemaphore(m_InternalSemaphoreID);
    m_InternalSemaphoreID = NULL;
    HX_DELETE(m_pMutex);
}

HX_RESULT
HXCarbonManualEvent::SignalEvent()
{
    m_pMutex->Lock();
    m_bIsSignalled = TRUE;
    ::MPSignalSemaphore(m_InternalSemaphoreID); // in case we're waiting.
    m_pMutex->Unlock();
    
    return HXR_OK;
}

HX_RESULT
HXCarbonManualEvent::ResetEvent()
{
    m_pMutex->Lock();
    m_bIsSignalled = FALSE;
    ::MPWaitOnSemaphore(m_InternalSemaphoreID, kDurationImmediate); // just clear it out...
    m_pMutex->Unlock();
    return HXR_OK;
}

HX_RESULT
HXCarbonManualEvent::Wait(UINT32 uTimeoutPeriod)
{
    HXBOOL bDone = FALSE;
    
    HX_RESULT retVal = HXR_OK;
    
    while (!bDone)
    {
	m_pMutex->Lock();
	HXBOOL bIsSignalled = m_bIsSignalled;
	m_pMutex->Unlock();
	if (bIsSignalled)
	{
	    bDone = TRUE;
	}
	else
	{
	    // xxxbobclark rely on MP semaphore
	    
	    Duration timeout = ( uTimeoutPeriod == ALLFS ) ? kDurationForever : ( uTimeoutPeriod * kDurationMillisecond );
	    
	    OSStatus osStatus = ::MPWaitOnSemaphore(m_InternalSemaphoreID, timeout);
	    bDone = TRUE;
	    if (osStatus == kMPTimeoutErr)
	    {
		retVal = HXR_WOULD_BLOCK;
	    }
	}
    }

    return retVal;
}

void*
HXCarbonManualEvent::GetEventHandle	(void)
{
    return (void*)this;
}


// HXCarbonMutex

HXCarbonMutex::HXCarbonMutex()
{
    MPCreateCriticalRegion(&mCriticalRegion);
}

HXCarbonMutex::~HXCarbonMutex()
{
    MPDeleteCriticalRegion(mCriticalRegion);
}

HX_RESULT
HXCarbonMutex::Lock(void)
{
    MPEnterCriticalRegion(mCriticalRegion, kDurationForever);
    return HXR_OK;
}

HX_RESULT
HXCarbonMutex::Unlock(void)
{
    MPExitCriticalRegion(mCriticalRegion);
    return HXR_OK;
}

HX_RESULT
HXCarbonMutex::Trylock(void)
{
    HX_RESULT retval = HXR_OK;
    OSStatus osStatus = MPEnterCriticalRegion(mCriticalRegion, kDurationImmediate);
    
    if (osStatus != noErr)
    {
    	retval = HXR_FAIL;
    } 
    return retval;
}


//HXCarbonSemaphore
HXCarbonSemaphore::HXCarbonSemaphore( UINT32 unInitialCount /*=0*/)
					:m_mpSemaphoreID(0)
{
   OSStatus osStatus = ::MPCreateSemaphore(INT_MAX, unInitialCount, &m_mpSemaphoreID);
   
   HX_ASSERT(osStatus == noErr);
}

HXCarbonSemaphore::~HXCarbonSemaphore()
{
	MPDeleteSemaphore(m_mpSemaphoreID);
}

HX_RESULT HXCarbonSemaphore::Post()
{
	HX_RESULT retval = HXR_OK;
	
	OSStatus osStatus = MPSignalSemaphore(m_mpSemaphoreID);
	
	if (osStatus != noErr)
	{
		retval = HXR_FAIL;
	}
	
	return retval; 
}

HX_RESULT HXCarbonSemaphore::Wait()
{
	HX_RESULT retval = HXR_OK;
	
	OSStatus osStatus = MPWaitOnSemaphore(m_mpSemaphoreID, kDurationForever);
	
	if (osStatus != noErr)
	{
		retval = HXR_FAIL;
	}
	
    	
	return retval;
}

HX_RESULT HXCarbonSemaphore::TryWait()
{
	HX_RESULT retval = HXR_OK;
	
	OSStatus osStatus = MPWaitOnSemaphore(m_mpSemaphoreID, kDurationImmediate);
	
	if (osStatus != noErr)
	{
		retval = HXR_FAIL;
	}
	
	return retval;
}
	
#if 0

// HXCarbonAsyncTimer

HXCarbonAsyncTimer::HXCarbonAsyncTimer(ULONG32 ulTimeOut, HXThread* pReceivingThread)
 : m_ulTimeout(ulTimeOut)
 , m_pReceivingThread(pReceivingThread)
 , m_CarbonTimerUPP(NULL)
 , m_CarbonTimerRef(NULL)
 , m_msg(NULL)
{
    m_msg = new HXThreadMessage(HXMSG_ASYNC_TIMER, (void*)m_ulTimeout, NULL, NULL);
    m_CarbonTimerUPP = ::NewEventLoopTimerUPP((EventLoopTimerProcPtr)MyCarbonTimer);
    ::InstallEventLoopTimer(GetCurrentEventLoop(), 0, kEventDurationMillisecond *
		m_ulTimeout, m_CarbonTimerUPP, this, &m_CarbonTimerRef);
}

HXCarbonAsyncTimer::~HXCarbonAsyncTimer()
{
    if (m_CarbonTimerRef)
    {
	::RemoveEventLoopTimer(m_CarbonTimerRef);
	m_CarbonTimerRef = NULL;
    }
    if (m_CarbonTimerUPP)
    {
	::DisposeEventLoopTimerUPP(m_CarbonTimerUPP);
	m_CarbonTimerUPP = NULL;
    }
    delete m_msg;
}

/* static */
UINT32
HXCarbonAsyncTimer::SetTimer(ULONG32 ulTimeOut, HXThread* pReceivingThread)
{
    volatile HXCarbonAsyncTimer* pAsyncTimer = new HXCarbonAsyncTimer(ulTimeOut, pReceivingThread);
    return (UINT32) pAsyncTimer;
}

/* static */
void
HXCarbonAsyncTimer::MyCarbonTimer(EventLoopTimerRef, HXCarbonAsyncTimer* pAsyncTimer)
{
    HX_ASSERT(pAsyncTimer);
    HX_ASSERT(pAsyncTimer->m_pReceivingThread);
    HXThreadMessage theMsg(HXMSG_ASYNC_TIMER, (void*)pAsyncTimer->m_ulTimeout, NULL, NULL);
    pAsyncTimer->m_pReceivingThread->PostMessage(&theMsg, NULL);
}

#endif

#endif
