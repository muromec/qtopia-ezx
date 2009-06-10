/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tthreads.cpp,v 1.5 2007/07/06 20:42:03 jfinnecy Exp $
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

#include <stdio.h>
#include <unistd.h> //for sleep
#include "hxassert.h"
#include "hxthread.h"
#include "hxthread.h"
#include "microsleep.h"
#include "hlxclib/stdlib.h"
#include "hxmsgs.h"  

extern "C"
{
    

//XXXgfw TODO
//
//  1) Add support for those advanced thread features some platforms
//     support (timed waits, message passing, named objects, etc).
//  2) Clean up all the memory leaks. If an error occurs I just return
//     without cleaning up at all.    

#define MAGIC_NUMBER 1234

//Global int for tests...
int nFlag = 0;
HXMutex* pMutex = NULL;

void* _threadFunc1(void* pData)
{
    fprintf( stderr, "Thread is running...\n");
    fprintf( stderr, "pData is %p and should match the magic number %p\n",
             pData, (void*)MAGIC_NUMBER);
    if( pData != (void*)MAGIC_NUMBER )
    {
        fprintf( stderr, "FAILED:Thread arg is wrong...\n" );
        exit(1);
    }
    
    sleep(3);
    return pData;
}

void* _threadFunc2(void* pData)
{
    HXMutex* pMutex = (HXMutex*)pData;
    fprintf( stderr, "Mutex test thread:  is running....\n" );
    fprintf( stderr, "Mutex test thread:  Going to lock mutex...\n" );
    nFlag = MAGIC_NUMBER;
    pMutex->Lock();
    nFlag = 0;
    fprintf( stderr, "Mutex test thread:  Got mutex...exiting...\n" );
    pMutex->Unlock();
    
    return (void*)66;
}

void* _threadFuncEvent1(void* pData)
{
    HXEvent* pEvent = (HXEvent*)pData;
    fprintf( stderr, "Event thread1:  is running....\n" );
    pEvent->Wait();
    pMutex->Lock();
    nFlag++;
    pMutex->Unlock();
    return (void*)0;
}

void* _threadFuncEvent2(void* pData)
{
    HXEvent* pEvent = (HXEvent*)pData;
    fprintf( stderr, "Event thread2:  is running....\n" );
    pEvent->Wait();
    pMutex->Lock();
    nFlag++;
    pMutex->Unlock();

    return (void*)0;
}

void _timerProc1( void*   hwnd,
                 UINT32  msg,
                 UINT32  id,
                 ULONG32 CurrentTime)
{
    fprintf( stderr, "id: %d  current time: %lu\n", id, CurrentTime );
    nFlag++;
}


typedef struct _stuff
{
    HXThread* pThread;
    HXEvent*  pEvent;
} ST_STUFF;


void* _timerProc2(void* pData)
{
    ST_STUFF* pTmp = (ST_STUFF*)pData;
    HXEvent*  pEvent = (HXEvent*)pTmp->pEvent;
    HXThread* pThread = (HXThread*)pTmp->pThread;
    HXThreadMessage msgTmp;
    
    fprintf( stderr, "_timerProc2: is running....\n" );
    while(1)
    {
        if( HXR_OK == pThread->GetMessage(&msgTmp))
        {
            //Got a message. If it is HXMSG_QUIT get our of here.
            if( msgTmp.m_ulMessage == HXMSG_QUIT )
            {
                fprintf( stderr, "Thread got quit message.\n" ); 
                break;
            }
            if( msgTmp.m_ulMessage == HXMSG_ASYNC_TIMER )
            {
                fprintf( stderr, "nFlag: %d\n", nFlag ); 
                nFlag++;
            }
            if( nFlag >= 5 )
            {
                fprintf( stderr, "Telling main thread to wake up...\n" ); 
                //Tell the master thread to wake up and kill us.
                pEvent->SignalEvent();
            }
            
        }
    }

    nFlag = 0;
    return (void*)0;
}


int main()
{
    fprintf( stderr, "Threads test running....\n" );

    ////////////////////////
    //
    // THREAD TESTS
    //
    ////////////////////////

    //Test thread creation.
    HX_RESULT res = HXR_OK;
    HXThread* pThread = NULL;
    
    res = HXThread::MakeThread(pThread);
    if( !pThread || FAILED(res) )
    {
        fprintf( stderr, "FAILED:Failed to make thread.\n" );
        return 1;
    }

    //Run the thread and make sure the two thread IDs are different.
    //ThreadFunc1 will sleep for 5 seconds. We can also test the basic
    //join at this point.
    pThread->CreateThread(_threadFunc1, (void*)MAGIC_NUMBER );

    //Thread is running, get the IDs..
    ULONG32 otherThread = 0;
    ULONG32 myThread = pThread->GetCurrentThreadID();
    pThread->GetThreadId(otherThread);

    if( 0==otherThread || otherThread==myThread || 0==myThread )
    {
        fprintf( stderr, "FAILED:Thread IDs should not match and should not be zero.\n" );
        return 1;
    }

    fprintf( stderr, "Thread ID 1: %lu   Thread ID 2: %lu\n", myThread, otherThread ); 

    //This should let us join on the other thread. In the current implementation
    //there is not way to return the exit value of the thread.
    fprintf( stderr, "Joining thread. Should be a 4-5 second wait...\n" ); 
    pThread->Exit(0);

    fprintf( stderr, "Back! Yeah...\n" );
    sleep(1);
    
    //Clean up the thread.
    HX_DELETE( pThread );

    
    ////////////////////////
    //
    // MUTEX TESTS
    //
    ////////////////////////
    HXMutex::MakeMutex(pMutex);

    //Lock it...
    fprintf( stderr, "Going to lock the mutex for the first time. If this dead\n" );
    fprintf( stderr, "locks...you did not pass the test..\n" );
    res = pMutex->Lock();
    if( FAILED(res) )
    {
        fprintf( stderr, "FAILED:Lock failed...\n" );
        return 1;
    }

    //Test to make sure its recursive as our mutexes must be...
    fprintf( stderr, "Going to lock the mutex for the second time. If this dead\n" );
    fprintf( stderr, "locks...you did not pass the test..\n" );
    res = pMutex->Lock();
    if( FAILED(res) )
    {
        fprintf( stderr, "FAILED: recursive Lock failed...\n" );
        return 1;
    }

    //Now we have the mutex locked 2 times, spawn a thread and make
    //sure it can't lock it.
    HXThread::MakeThread(pThread);
    if( !pThread || FAILED(res) )
    {
        fprintf( stderr, "FAILED:Failed to make thread for mutex test.\n" );
        return 1;
    }
    //Thread func 2 will just try and lock the mutex we pass in.
    //It better block until we release it 2 times...
    nFlag = 0;
    pThread->CreateThread(_threadFunc2, (void*)pMutex );
    
    //make sure the thread has a chance to run and set the magic number.
    sleep(1);
    fprintf( stderr, "test thread should now be blocked...\n" );
    if( nFlag != MAGIC_NUMBER )
    {
        fprintf( stderr, "FAILED:test thread did no block on locked mutex...\n" );
        return 1;
    }
    
    fprintf( stderr, "Unlocking the mutex once...test thread should not be free.\n" );
    pMutex->Unlock();
    sleep(1);

    if( nFlag != MAGIC_NUMBER )
    {
        fprintf( stderr, "FAILED:mutex lock count does not work...\n" );
        return 1;
    }
    
    fprintf( stderr, "Unlocking the mutex again. This should free test thread..\n" );
    pMutex->Unlock();
    sleep(1);
    if( nFlag != 0 )
    {
        fprintf( stderr, "FAILED: test thread should have been unlocked....\n" );
        return 1;
    }
    
    HX_DELETE( pThread );
    HX_DELETE( pMutex );
    
        
    ////////////////////////
    //
    // EVENT TESTS
    //
    ////////////////////////
     HXEvent* pEvent = NULL;
    HXEvent::MakeEvent(pEvent, "foo", FALSE);

    if( !pEvent )
    {
        fprintf( stderr, "FAILED: Can't make events...\n" );
        return 1;
    }
    
    //Non manual resets events. Only one thread should wake up each signal.
     HXThread* pThread1 = NULL;
    HXThread* pThread2 = NULL;
    
    HXThread::MakeThread(pThread1);
    HXThread::MakeThread(pThread2);
    HXMutex::MakeMutex(pMutex); //to protect out flag.
    if( !pThread1 || !pThread2 || !pMutex )
    {
        fprintf( stderr, "FAILED:Failed to make thread for Event test.\n" );
        return 1;
    }
    nFlag = 0;
    
    //Thread func 2 will just try and lock the mutex we pass in.
    //It better block until we release it 2 times...
    res = pThread1->CreateThread(_threadFuncEvent1, (void*)pEvent );
    if( FAILED(res) )
    {
        fprintf( stderr, "FAILED: Can't create Event Thread1\n" );
        return 1;
    }
    
    res = pThread2->CreateThread(_threadFuncEvent2, (void*)pEvent );
    if( FAILED(res) )
    {
        fprintf( stderr, "FAILED: Can't create Event Thread2\n" );
        return 1;
    }

    sleep(1);
    //Both threads should be waiting on us to signal the event then.
    if( nFlag != 0 )
    {
        fprintf( stderr, "FAILED: nFlag should be zero.\n" );
        return 1;
    }
    //Let one thread go...
    pEvent->SignalEvent();
    sleep(1);
    if( nFlag != 1 )
    {
        //Either the thread didn't go or both went. Either way its
        //bad.
        fprintf( stderr, "FAILED: nFlag should be 1.\n" );
        return 1;
    }

    //Let the next thread go...
    pEvent->SignalEvent();
    sleep(1);
    if( nFlag != 2 )
    {
        //Either the thread didn't go or both went. Either way its
        //bad.
        fprintf( stderr, "FAILED: nFlag should be 2.\n" );
        return 1;
    }

    HX_DELETE(pEvent);
    HX_DELETE(pThread1);
    HX_DELETE(pThread2);
    //Now test manual reset ones...
    HXEvent::MakeEvent(pEvent, "foo", TRUE);

    if( !pEvent )
    {
        fprintf( stderr, "FAILED: Can't make events...\n" );
        return 1;
    }
    
    //Non manual resets events. Only one thread should wake up each signal.
    HXThread::MakeThread(pThread1);
    HXThread::MakeThread(pThread2);
    if( !pThread1 || !pThread2 || !pMutex )
    {
        fprintf( stderr, "FAILED:Failed to make thread for Event test.\n" ); 
        return 1;
    }
    nFlag = 0;
    
    //Thread func 2 will just try and lock the mutex we pass in.
    //It better block until we release it 2 times...
    pThread1->CreateThread(_threadFuncEvent1, (void*)pEvent );
    pThread2->CreateThread(_threadFuncEvent2, (void*)pEvent );

    sleep(1);
    //Both thread should be waiting on the broadcast...
    if( nFlag != 0 )
    {
        fprintf( stderr, "FAILED: nFlag should be zero.\n" );
        return 1;
    }
    //Let all threads go...
    pEvent->SignalEvent();
    sleep(1);
    
    if( nFlag != 2 )
    {
        fprintf( stderr, "FAILED: both threads should have gone..\n" );
        return 1;
    }
    
    HX_DELETE(pEvent);
    HX_DELETE(pThread1);
    HX_DELETE(pThread2);
    HX_DELETE(pMutex);


    ////////////////////////
    //
    // ASYNC TIMER TESTS
    //
    ////////////////////////

    //Try a timer that calls a proc first....
    fprintf( stderr, "You should see our timer proc fire every second...\n" );
    nFlag = 0;
    int id = HXAsyncTimer::SetTimer( 1000, _timerProc1 );

    sleep(5);
    HXAsyncTimer::KillTimer(id);
    int ttt = nFlag;
    fprintf( stderr, "You should see no more timer procs....\n" ); 
    sleep(3);

    //The timer proc incrments nFlag each time it is called. So, since
    //we slept for 5 seconds and we were suppose to call it every second
    //I will take it as a fail if it hasn't at least been incremnted
    //at least once. Also, if nFlag gets incremented again it didn't kill
    //correctly.
    if( 0==ttt )
    {
        fprintf( stderr, "FAILED: The timer proc didn't ever fire.\n" );
        return 1;
    }
    if( nFlag!=ttt )
    {
        fprintf( stderr, "FAILED: The timer proc didn't stop firing when we killed it.\n" );
        return 1;
    }
    
    
    //Now try posting timer messages to a thread....
    HXThread::MakeThread(pThread1);
    if( !pThread1  )
    {
        fprintf( stderr, "FAILED:Failed to make thread for Timer test.\n" ); 
        return 1;
    }
    nFlag = 0;
    
    //Send it a few messages. When it gets 5 messages it will signal
    //this event that we are waiting on and then we will kill the timer
    //and send it a quit event to it can die.
    HXEvent::MakeEvent(pEvent, "foo", TRUE);
    if( !pEvent )
    {
        fprintf( stderr, "FAILED: Can't make event for timer test.\n" );
        return 1;
    }

    ST_STUFF stuff;
    stuff.pEvent = pEvent;
    stuff.pThread = pThread1;
    
        
    pThread1->CreateThread(_timerProc2, (void*)&stuff );
    id = HXAsyncTimer::SetTimer( 1000, pThread1 );
    pEvent->Wait();
    //Verify that the thread got at least 5 async messages.
    if( nFlag < 5 )
    {
        fprintf( stderr, "FAILED: _timerProc2 didn't get enough messages.\n" );
        return 1;
    }
    
    //Kill the timer before we destroy the thread.
    HXAsyncTimer::KillTimer(id);
    
    //When the thread exits it will set the nFlag back to zero...
    HXThreadMessage msgQuit(HXMSG_QUIT, NULL, NULL);
    pThread1->PostMessage( &msgQuit );

    //Join the thread.
    pThread1->Exit(0);
    if( nFlag != 0 )
    {
        fprintf( stderr, "FAILED: Thread didn't quit correctly\n" );
        return 1;
    }
    HX_DELETE( pThread1 );

    fprintf( stderr, "PASSED!\n" ); 
    return 0;
}



}
