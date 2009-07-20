/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: UnixThreads.cpp,v 1.19 2006/08/16 17:30:13 gwright Exp $
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

//This is used to turn off threads in libc5 builds.
#ifdef _UNIX_THREADS_SUPPORTED

#include "hxtypes.h" //for UINT32
#include "hxresult.h" //for HX_RESULT

#include <errno.h>
#include "UnixThreads.h"

#include <signal.h>   //for SIGKILL
#include "hxassert.h" //for HX_ASSERT
#include "microsleep.h"
#include "hxmsgs.h" //for HXMSG_ASYNC_TIMER message.
#include "hxtick.h"  //for GetTickCount()
#include "pckunpck.h"


//=================================================================
// In this section here include the OS specific headerfiles for
// each implementation.
//=================================================================
#if defined( _LINUX ) || defined(_HPUX) || defined(_MAC_UNIX)
#include <pthread.h>
#include <semaphore.h>
#include "pthreadthreads.h"
#endif

#if defined( _SOLARIS )
#include <synch.h>
#include <thread.h>
#include "solaristhreads.h"
#endif

#if defined( _IRIX )
#endif

#if defined( _AIX )
#endif

//=======================================================================
//
//                      HXUnixThread
//                   ------------------
//
//=======================================================================
HXUnixThread::HXUnixThread() 
    : m_threadID(0),
      m_messageQue(),
      m_pCond(NULL),
      m_pCondLock(NULL)
{
    //Create the condition with its associated mutex.
    //Do NOT delete the mutex that the cond makes for us....
    HXUnixCondition::MakeCondition(m_pCond, m_pCondLock );
    HX_ASSERT( m_pCondLock && m_pCond);
}

HXUnixThread::~HXUnixThread()
{
    if( m_threadID != 0 )
    {
        //XXXgfw Had to get rid of this. Some threads are destroyed on their own thread.
        //HX_ASSERT( "Thread has not been joined! Memory/mutex leaks????\n" == NULL );
        //We can try to cancel here but it won't work!
        //The thread may not even honor the cancel request. If it does and has not
        //set up cancelation/clean-up routine you better hope that it doesn't have
        //any mutexes locked or resources open!!! Just don't do it.
    }

    //Clean up message que.
    while( !m_messageQue.IsEmpty() )
    {
        HXThreadMessage* pTmp = (HXThreadMessage *)(m_messageQue.RemoveHead());
        HX_DELETE( pTmp );
    }

    HX_DELETE( m_pCond );
    m_pCondLock = NULL;
}

HX_RESULT HXUnixThread::MakeThread(HXThread*& pThread )
{
#if defined( _LINUX ) || defined (_HPUX) || defined(_MAC_UNIX)
    pThread = new HXPthreadThread();
#elif defined( _SOLARIS )
    pThread = new HXSolarisThread();
#else
    HX_ASSERT( "No unix thread for this platform" == NULL );
#endif    
    if(pThread == NULL)
    {
        HX_ASSERT( 0 );
	return HXR_OUTOFMEMORY;
    }
    
    return HXR_OK;
}

//ulCreationFlags is ignored. We can't suspend pthreads and that is the only
//flag possible right now.
HX_RESULT HXUnixThread::CreateThread( void*(pfExecFunc(void*)), void* pArg, ULONG32 ulCreationFlags)
{
    HX_RESULT retVal = HXR_OK;
    
    HX_ASSERT( m_threadID==0 );
    if( m_threadID != 0)
    {
        retVal = HXR_UNEXPECTED;
    }
    else
    {
        if( _thread_create( m_threadID, pfExecFunc, pArg ) != HXR_OK )
        {
            retVal = HXR_FAIL;
            m_threadID = 0;
        }
    }

    return retVal;
}


HX_RESULT HXUnixThread::Exit(UINT32 unExitCode)
{
    //Make sure that only the thread that was created it calling the exit
    //routine. If the parent thread wants to kill it maybe we should have
    //a Kill method or something.
    if( 0 == m_threadID )
    {
        //Thread has already gone. Just return.
        return HXR_UNEXPECTED;
    }
    
    if( m_threadID != GetCurrentThreadID() )
    {
        //Ok, because of the way winthrd.cpp does it, this call also
        //acts as a 'pthread_join' when the calling thread isn't the
        //m_threadID.
        
        //Also, it looks like this method isn't set up to look at the
        //return value of the thread. We could return HXR_FAIL is it
        //is anything except 0 but for now just throw it away.
        Terminate();
    }
    else
    {
        _thread_exit( unExitCode );
    }
    
    return HXR_OK;
}


HX_RESULT HXUnixThread::SetPriority( UINT32 ulPriority)
{
    //XXXGFW not supported yet.
//     struct sched_param stThreadParams;
//     int    nPolicy = 0;
//     int    nRet    = -1;

//     HX_ASSERT( m_threadID );

//     if( m_threadID != 0)
//     {
//         nPolicy = SCHED_OTHER;
//         stThreadParams.sched_priority = (int)ulPriority; 
        
//         nRet = pthread_setschedparam( m_threadID, nPolicy, &stThreadParams );
//     }
    
//     return nRet==0 ? HXR_OK : HXR_FAIL;
    return HXR_OK;
}

//XXXgfw not tested yet!!!!!!! ...but should work. :)
HX_RESULT HXUnixThread::GetPriority( UINT32& ulPriority)
{
    //XXXGFW not supported yet.....
//     struct sched_param stThreadParams;
//     int    nPolicy = 0;
//     int    nRet    = -1;
    
//     HX_ASSERT( m_threadID );
    
//     if( m_threadID != 0)
//     {
//         nRet = pthread_getschedparam( m_threadID, &nPolicy, &stThreadParams );
//         ulPriority = stThreadParams.sched_priority;
//     }
    
//     return nRet==0 ? HXR_OK : HXR_FAIL;
    return HXR_OK;
}

HX_RESULT HXUnixThread::YieldTimeSlice()
{
    microsleep(1);
    return HXR_OK;
}


HX_RESULT HXUnixThread::GetThreadId(UINT32& ulThreadId)
{
    ulThreadId = m_threadID;
    return HXR_OK;
}

//Cancels the thread that was created.
HX_RESULT HXUnixThread::Cancel()
{
    HX_ASSERT( "This isn't portable. Don't use it" == NULL );
    if( 0 == m_threadID )
    {
        return HXR_UNEXPECTED;
    }
    _thread_cancel(m_threadID);
    return HXR_OK;
}

//Must wait for our child thread to exit and return the ret code.
HX_RESULT HXUnixThread::Terminate()
{
    ULONG32 ulRetVal = 0;

    if( 0 == m_threadID || GetCurrentThreadID() == m_threadID )
    {
        return HXR_UNEXPECTED;
    }

    ulRetVal = _thread_join( m_threadID );

    //Thread has exited and we are done.
    m_threadID = 0;
    
    return (ulRetVal == 0)? HXR_OK : HXR_FAILED;
}

HX_RESULT HXUnixThread::Suspend()
{
    HX_ASSERT( "HXUnixThread::Suspend is not implemented yet." == NULL );
    return HXR_FAIL;
}

HX_RESULT HXUnixThread::Resume()
{
    HX_ASSERT( "HXUnixThread::Resume  is not implemented yet." == NULL );
    return HXR_FAIL;
}

HX_RESULT HXUnixThread::PostMessage(HXThreadMessage* pMsg, void* pWindowHandle)
{
    HX_RESULT retVal = HXR_OK;

    //Assert that we don't use pWindowHandle. 
    HX_ASSERT( pWindowHandle == NULL );
    
    //To mimic the windows PostMessage we must COPY the pMsg and put
    //it on our que. pMsg is going to go out of scope most likely
    if( NULL != pMsg )
    {
        HXThreadMessage *pMsgTmp = new HXThreadMessage(pMsg);
        if( pMsgTmp == NULL )
        {
            retVal = HXR_OUTOFMEMORY;
        }
        else
        {
            //Lock the mutex protecting the message que.
            m_pCondLock->Lock();
            m_messageQue.AddTail((void*)pMsgTmp);
            
            //If we were empty the the GetMessage thread could have been waiting 
            //on us to post. Signal it.
            m_pCond->Signal();
            m_pCondLock->Unlock();
        }
    }
    
    return retVal;
}

HX_RESULT HXUnixThread::GetMessage( HXThreadMessage* pMsg, 
                                     UINT32 ulMsgFilterMin, 
                                     UINT32 ulMsgFilterMax)
{
    HX_RESULT retVal = HXR_OK;

    //assert that ulMsgFilterMax/Min is zero as we don't support it yet.
    HX_ASSERT( ulMsgFilterMax == 0 && ulMsgFilterMin == 0 );
    HX_ASSERT( pMsg );
    
    //We must pop the next message, COPY it into pMsg and delete our copy.
    if( pMsg != NULL )
    {
        //Get access
        m_pCondLock->Lock();

        //If the que is empty we need to block and wait.
        while( m_messageQue.IsEmpty() )
        {
            m_pCond->Wait();
        }

        if( !m_messageQue.IsEmpty())
        {
            HXThreadMessage* pMsgTmp = (HXThreadMessage*)m_messageQue.RemoveHead();
            pMsg->m_ulMessage             = pMsgTmp->m_ulMessage; 
            pMsg->m_pParam1               = pMsgTmp->m_pParam1; 
            pMsg->m_pParam2               = pMsgTmp->m_pParam2;
            pMsg->m_pPlatformSpecificData = pMsgTmp->m_pPlatformSpecificData;
            
            //free it.
            HX_DELETE( pMsgTmp );
        }
        else
        {
            HX_ASSERT( "que panic" == NULL );
        }
        m_pCondLock->Unlock();
    }
    
    return retVal;
}
HX_RESULT HXUnixThread::PeekMessageMatching( HXThreadMessage* pMsg,
                                             HXThreadMessage* pMatch,
                                             HXBOOL bRemoveMessage )
{
    HX_RESULT retVal = HXR_OK;
        
    HX_ASSERT( pMsg );
    HX_ASSERT( pMatch );
     
    if( pMsg != NULL && pMatch!=NULL )
    {
        //Protect the que.
        m_pCondLock->Lock();

        if( !m_messageQue.IsEmpty() )
        {
            HXThreadMessage* pMsgTmp = NULL;

            //Loop throught the messages and find a matching
            //one.
            HXBOOL bSkipMessage  = (pMatch->m_ulMessage==0);
            HXBOOL bSkipParam1   = (pMatch->m_pParam1==NULL);
            HXBOOL bSkipParam2   = (pMatch->m_pParam2==NULL);
            HXBOOL bSkipPlatform = (pMatch->m_pPlatformSpecificData==NULL);
            CHXSimpleList::Iterator i;
            
            for( i=m_messageQue.Begin(); i!=m_messageQue.End(); ++i)
            {
                pMsgTmp = (HXThreadMessage*)(*i);

                //Does it match?
                if( bSkipMessage || pMatch->m_ulMessage==pMsgTmp->m_ulMessage )
                    if( bSkipParam1 || pMatch->m_pParam1==pMsgTmp->m_pParam1 )
                        if( bSkipParam2 || pMatch->m_pParam2==pMsgTmp->m_pParam2 )
                            if( bSkipPlatform || pMatch->m_pPlatformSpecificData==pMsgTmp->m_pPlatformSpecificData )
                                break;
            }
            //Did we find a match?
            if( i != m_messageQue.End())
            {
                //We found one!
                pMsg->m_ulMessage             = pMsgTmp->m_ulMessage; 
                pMsg->m_pParam1               = pMsgTmp->m_pParam1; 
                pMsg->m_pParam2               = pMsgTmp->m_pParam2;
                pMsg->m_pPlatformSpecificData = pMsgTmp->m_pPlatformSpecificData;
                
                //Only free it if we removed it from the queue.
                if( bRemoveMessage )
                {
                    //XXXgfw That has to be a better way than this. We
                    //have the iterator up above. How do you delete with
                    //one.
                    LISTPOSITION listpos = m_messageQue.Find(pMsgTmp);
                    HX_ASSERT( listpos );
                    if(listpos)
                    {
                        m_messageQue.RemoveAt(listpos);
                    }
                    HX_DELETE( pMsgTmp );
                }
            }
            else
            {
                retVal=HXR_FAIL;
            }
        }
        else
        {
            //There was no message to get
            retVal=HXR_FAIL;
        }
        m_pCondLock->Unlock();
    }
    return retVal;
}

HX_RESULT HXUnixThread::PeekMessage( HXThreadMessage* pMsg,
                                      UINT32 ulMsgFilterMin,
                                      UINT32 ulMsgFilterMax,
                                      HXBOOL   bRemoveMessage
                                      )
{
    HX_RESULT retVal = HXR_OK;

    //assert that ulMsgFilterMax/Min is zero as we don't support it yet.
    HX_ASSERT( ulMsgFilterMax == 0 && ulMsgFilterMin == 0 );
    HX_ASSERT( pMsg );
    
    //We must pop the next message, COPY it into pMsg and delete our copy.
    if( pMsg != NULL )
    {
        //Protect the que.
        m_pCondLock->Lock();
        if( !m_messageQue.IsEmpty() )
        {
            HXThreadMessage* pMsgTmp = NULL;

            //Do we romove the message or peek at it?
            if( bRemoveMessage )
            {
                pMsgTmp = (HXThreadMessage*)m_messageQue.RemoveHead();
            }
            else
            {
                pMsgTmp = (HXThreadMessage*)m_messageQue.GetHead();
            }
                
            if( pMsgTmp != NULL )
            {
                pMsg->m_ulMessage             = pMsgTmp->m_ulMessage; 
                pMsg->m_pParam1               = pMsgTmp->m_pParam1; 
                pMsg->m_pParam2               = pMsgTmp->m_pParam2;
                pMsg->m_pPlatformSpecificData = pMsgTmp->m_pPlatformSpecificData;
                
                //Only free it if we removed it from the queue.
                if( bRemoveMessage )
                    HX_DELETE( pMsgTmp );
            }
            else
            {
                HX_ASSERT( "que panic" == NULL );
            }
        }
        else
        {
            //There was no message to get
            retVal=HXR_FAIL;
        }
        m_pCondLock->Unlock();
    }
    return retVal;
}
HX_RESULT HXUnixThread::DispatchMessage(HXThreadMessage* pMsg)
{
    HX_ASSERT( "HXUnixThread::DispatchMessage is not implemented yet." == NULL );
    return HXR_FAIL;
}








//=======================================================================
//
//                      HXUnixMutex
//                   ------------------
//
//=======================================================================
HXUnixMutex::HXUnixMutex()
{
}
    
HXUnixMutex::~HXUnixMutex()
{
}

HX_RESULT HXUnixMutex::MakeMutex( HXMutex*& pMutex )
{
#if defined( _LINUX ) || defined(_HPUX) || defined(_MAC_UNIX)
    pMutex = new HXPthreadMutex();
#elif defined( _SOLARIS )
    pMutex = new HXSolarisMutex();
#else
    HX_ASSERT( "No unix mutex for this platform" == NULL );
#endif
    if(pMutex == NULL)
    {
        HX_ASSERT( 0 );
	return HXR_OUTOFMEMORY;
    }
    
    return HXR_OK;

}


HX_RESULT HXUnixMutex::Lock()
{
    return _Lock();
}
    
HX_RESULT HXUnixMutex::Unlock()
{
    return _Unlock();
}
    
HX_RESULT HXUnixMutex::Trylock()
{
    return _TryLock();
}


//=======================================================================
//
//                      HXUnixSemaphore
//                   ------------------
//
//=======================================================================

HXUnixSemaphore::HXUnixSemaphore(UINT32 unInitialCount)
    : m_unInitialCount(unInitialCount)
{
}

HXUnixSemaphore::~HXUnixSemaphore()
{
}

HX_RESULT HXUnixSemaphore::MakeSemaphore(HXUnixSemaphore*& pSem)
{
#if defined( _LINUX ) || defined(_HPUX)
    pSem = new HXPthreadSemaphore();
#elif defined(_MAC_UNIX)
    pSem = new HXPthreadMacSemaphore();
#elif defined( _SOLARIS )
    pSem = new HXSolarisSemaphore();
#else
    HX_ASSERT( "No unix semaphore for this platform" == NULL );
#endif
    if(pSem == NULL)
    {
        HX_ASSERT(0);
	return HXR_OUTOFMEMORY;
    }
    
    return HXR_OK;
}

 
HX_RESULT HXUnixSemaphore::Post()
{
    return _Post();
}

HX_RESULT HXUnixSemaphore::Wait()
{
    return _Wait();
}

HX_RESULT HXUnixSemaphore::TryWait()
{
    return _TryWait();
}

HX_RESULT HXUnixSemaphore::GetValue( int* pnCount)
{
    return _GetValue( pnCount );
}



//=======================================================================
//
//                      HXUnixEvent
//                   ------------------
//
//=======================================================================   
HXUnixEvent::HXUnixEvent(const char* pEventName, HXBOOL bManualReset)
    : m_bIsManualReset( bManualReset ),
      m_bEventIsSet(FALSE),
      m_pCondLock(NULL),
      m_pCond(NULL)
{
    //
    //  NOTE: Because of the way the windows Cond vars work we have:
    //
    //   bManualReset==1  Once we signal once, all other signal/wait
    //                    calls are noops. All threads awake.
    //   bManualReset==0  Once signaled we retain until someone Waits.
    //                    Once someone waits, only one thread wakes up
    //                    and the signal is reset.
    //

    //Create the condition with its associated mutex.
    //Do NOT delete the mutex that the cond makes for us....
    HXUnixCondition::MakeCondition(m_pCond, m_pCondLock );
    HX_ASSERT( m_pCondLock && m_pCond);
}

HXUnixEvent::~HXUnixEvent()
{
    //Do NOT delete the mutex that the cond makes for us....
    HX_DELETE( m_pCond );
    m_pCondLock = NULL;
}

HX_RESULT HXUnixEvent::SignalEvent()
{
    //Lock it all down.
    m_pCondLock->Lock();

    //Whether or not this is manual reset, set the state.
    m_bEventIsSet = TRUE;

    //Signal the event depending on what type it is.
    if( m_bIsManualReset )
    {
        //Manual reset, wake up all threads. All waits become noops
        //until the event is reset.
        m_pCond->Broadcast();
    }
    else
    {
        m_pCond->Signal();
    }
    
    //Unlock it and go.
    m_pCondLock->Unlock();

    return HXR_OK;
}

HX_RESULT HXUnixEvent::Wait( UINT32 uTimeoutPeriod )
{
    HX_RESULT res = HXR_OK;
    
    m_pCondLock->Lock();
    //Check to see if this event has already been signaled.
    if( m_bEventIsSet )
    {
        //If we are not manual reset and we are signaled. reset the
        //signaled flag before returning.
        if( !m_bIsManualReset )
        {
            m_bEventIsSet = FALSE;
        }
        m_pCondLock->Unlock();
        return HXR_OK;
    }
    
    //We are not manual reset.
    if(uTimeoutPeriod!=ALLFS)
    {
        //XXXgfw We can be woken up by signals before the event is
        //actually signaled or the time has elapsed. Not sure what to
        //do in that case yet. Since most implementations try to
        //minimize these wakups I will just ignore it for now since we
        //are just using condtionals to mimic Window's events.
        res = m_pCond->TimedWait(uTimeoutPeriod);
    }
    else
    {
        m_pCond->Wait();
    }

    //Now, if we just woke up and the event had been signaled, We need
    //reset the event.
    if( !m_bIsManualReset && m_bEventIsSet )
    {
        m_bEventIsSet = FALSE;
    }

    //Now that we have waited
    m_pCondLock->Unlock();
    return res;
}

HX_RESULT HXUnixEvent::ResetEvent()
{
    m_pCondLock->Lock();
    m_bEventIsSet = FALSE;
    m_pCondLock->Unlock();
    return HXR_OK;
}


void* HXUnixEvent::GetEventHandle()
{
    //XXXgfw This doesn't look like it is used right now. Assuming we
    //just want it to be a unique handle to this event, we can use the
    //this pointer to the cond class we made.
    return (void*)m_pCond;
}

//=======================================================================
//
//                      HXUnixCondition
//                   ---------------------
//
//=======================================================================
HXUnixCondition::HXUnixCondition()
{
};

HXUnixCondition::~HXUnixCondition()
{
};



HX_RESULT HXUnixCondition::MakeCondition( HXUnixCondition*& pCond,
                                          HXUnixMutex*&     pMutex )
{
    HX_ASSERT( pMutex==NULL);
    
#if defined( _LINUX ) && defined(_UNIX_THREADS_SUPPORTED) || defined(_MAC_UNIX) || defined( _HPUX )
    pCond = (HXUnixCondition*) new HXPthreadCondition(pMutex);
#elif defined(_SOLARIS)
    pCond = (HXUnixCondition*) new HXSolarisCondition(pMutex);
#else
    HX_ASSERT( "No unix condtional for this platform" == NULL );
#endif
    if(pCond == NULL )
    {
        HX_ASSERT(0);
        return HXR_OUTOFMEMORY;
    }
    HX_ASSERT( pMutex );
    return HXR_OK;
}

HX_RESULT HXUnixCondition::Wait()
{
    HX_RESULT ret;
    ret = _Wait();
    return ret;
}

HX_RESULT HXUnixCondition::TimedWait( UINT32 uTimeoutPeriod )
{
    return _TimedWait(uTimeoutPeriod);
}

HX_RESULT HXUnixCondition::Broadcast()
{
    return _Broadcast();
}

HX_RESULT HXUnixCondition::Signal()
{
    return _Signal();
}

//=======================================================================
//
//                      HXUnixAsyncTimer
//                   -----------------------
//
//=======================================================================   


//Static data initializers
HXMutex*       HXUnixAsyncTimer::m_pmtxMapLock = NULL;
CHXMapLongToObj HXUnixAsyncTimer::m_mapTimers;

//Timeouts are in miliseconds.
HXUnixAsyncTimer::HXUnixAsyncTimer( ULONG32 ulTimeOut, HXThread* pReceivingThread )
    : m_ulTimeOut( ulTimeOut ),
      m_pReceivingThread( pReceivingThread ),
      m_pMessagePump( NULL ),
      m_pMsg(NULL),
      m_pfExecFunc(NULL)
{
    //Make the message to pump.
    m_pMsg = new HXThreadMessage( HXMSG_ASYNC_TIMER, (void*)m_ulTimeOut, NULL, NULL );
    
    //Start the thread. We have to do this weird casting because
    //the HXThread::MakeThread takes HXThread*&....
    HXThread* pTmp = NULL;
    HXUnixThread::MakeThread(pTmp);
    HX_ASSERT( pTmp );
    m_pMessagePump = (HXUnixThread*)pTmp;
    m_pMessagePump->CreateThread( _ActualMessagePump, (void*)this );
}

HXUnixAsyncTimer::HXUnixAsyncTimer( ULONG32 ulTimeOut, TIMERPROC pfExecFunc )
    : m_ulTimeOut( ulTimeOut ),
      m_pReceivingThread(NULL),
      m_pMessagePump(NULL),
      m_pMsg(NULL),
      m_pfExecFunc( pfExecFunc )
{
    //we need non-null pfExecFunc
    HX_ASSERT( m_pfExecFunc != NULL );
    
    //Start the thread.
    HXThread* pTmp = NULL;
    HXUnixThread::MakeThread(pTmp);
    HX_ASSERT( pTmp );
    m_pMessagePump = (HXUnixThread*)pTmp;
    m_pMessagePump->CreateThread( _ActualMessagePump, (void*)this );
}

HXUnixAsyncTimer::~HXUnixAsyncTimer()    
{
   //Tell the message pump to quit.
    HXThreadMessage msgQuit(HXMSG_QUIT, NULL, NULL);
    m_pMessagePump->PostMessage( &msgQuit );

    //Wait for it to stop.
    m_pMessagePump->Terminate();
    HX_DELETE( m_pMessagePump );
    HX_DELETE(m_pMsg);
}

ULONG32 HXUnixAsyncTimer::GetID()
{
    ULONG32 ulTmp=0;
    m_pMessagePump->GetThreadId(ulTmp);
    return ulTmp;
}


//XXXgfw just to keep the below more readable.
#define PARG ((HXUnixAsyncTimer*)pArg)
void* HXUnixAsyncTimer::_ActualMessagePump(void* pArg)
{
    while(1)
    {
        if( HXR_OK == PARG->m_pMessagePump->PeekMessage(&PARG->m_msgTmp))
        {
            //Got a message. If it is HXMSG_QUIT get our of here.
            if( PARG->m_msgTmp.m_ulMessage == HXMSG_QUIT )
            {
                break;
            }
        }
        
        microsleep( PARG->m_ulTimeOut*1000 );

        if( PARG->m_pMsg != NULL )
            PARG->m_pReceivingThread->PostMessage( PARG->m_pMsg);
        else
            PARG->m_pfExecFunc( 0, 0, PARG->GetID(), GetTickCount() );
    }
    return NULL;
}
#undef PARG

UINT32 HXUnixAsyncTimer::SetTimer(ULONG32 ulTimeOut, HXThread* pReceivingThread )
{
    if( m_pmtxMapLock == NULL )
    {
        HXMutex::MakeMutex( m_pmtxMapLock );
        HX_ASSERT( m_pmtxMapLock );
    }
    
    //lock it.
    m_pmtxMapLock->Lock();

    ULONG32 ulTimerID = 0;
    
    HX_ASSERT( ulTimeOut != 0 );
    HX_ASSERT( pReceivingThread != NULL );
    
    HXUnixAsyncTimer* pTimer = new HXUnixAsyncTimer(ulTimeOut, pReceivingThread );
    HX_ASSERT( pTimer != NULL );
    if( pTimer != NULL )
    {
        //Add new timer to map.
        ulTimerID = pTimer->GetID();
        m_mapTimers.SetAt( ulTimerID, (void*)pTimer );    
    }
    
    //unlock the map.
    m_pmtxMapLock->Unlock();

    return ulTimerID;
}

UINT32 HXUnixAsyncTimer::SetTimer(ULONG32 ulTimeOut, TIMERPROC pfExecFunc )
{
    if( m_pmtxMapLock == NULL )
    {
        HXMutex::MakeMutex( m_pmtxMapLock );
        HX_ASSERT( m_pmtxMapLock );
    }
    
    //lock it.
    m_pmtxMapLock->Lock();

    ULONG32 ulTimerID = 0;
    
    HX_ASSERT( ulTimeOut != 0 );
    HX_ASSERT( pfExecFunc != NULL );
    
    HXUnixAsyncTimer* pTimer = new HXUnixAsyncTimer(ulTimeOut, pfExecFunc );
    HX_ASSERT( pTimer != NULL );
    if( pTimer != NULL )
    {
        //Add new timer to map.
        ulTimerID = pTimer->GetID();
        m_mapTimers.SetAt( ulTimerID, (void*)pTimer );    
    }
    
    //unlock the map.
    m_pmtxMapLock->Unlock();

    return ulTimerID;
}

HXBOOL HXUnixAsyncTimer::KillTimer(UINT32 ulTimerID )
{
    //lock it.
    m_pmtxMapLock->Lock();

    HXBOOL  bRetVal = FALSE;
    void* pTimer  = NULL;


    HX_ASSERT( ulTimerID != 0 );
    
    if( m_mapTimers.Lookup( ulTimerID, pTimer ) )
    {
        //Found it.
        bRetVal = TRUE;
        HXUnixAsyncTimer* pTmp = (HXUnixAsyncTimer*)pTimer;
        HX_DELETE(pTmp); 
        m_mapTimers.RemoveKey( ulTimerID );
    }

    //unlock the map.
    m_pmtxMapLock->Unlock();

    return bRetVal;
}




#endif //_UNIX_THREADS_SUPPORTED
