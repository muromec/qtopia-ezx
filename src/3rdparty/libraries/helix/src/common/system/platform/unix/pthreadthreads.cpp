/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pthreadthreads.cpp,v 1.11 2005/03/02 20:38:54 grobbins Exp $
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

#include <errno.h>
#include "hxtypes.h"
#include "hxresult.h"

#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>
#include "pthreadthreads.h"


ULONG32 HXUnixThread::GetCurrentThreadID()
{
    return (ULONG32) pthread_self();
}

//=======================================================================
//
//                      HXPthreadThread
//                   ----------------------
//
//=======================================================================

HXPthreadThread::HXPthreadThread()
    : HXUnixThread()
{}

HXPthreadThread::~HXPthreadThread()
{}

HX_RESULT HXPthreadThread::_thread_create( ULONG32& ulThreadID, void*(pfExecFunc(void*)), void* pArg )
{
    HX_RESULT retVal = HXR_OK;
    pthread_t threadID=0;
    int nCode = pthread_create( &threadID, NULL, pfExecFunc, pArg );
    ulThreadID = (ULONG32) threadID;
    if(nCode!=0)
    {
        ulThreadID = 0;
        retVal = HXR_FAIL;
        HX_ASSERT( "Failed to create thread"==NULL );
    }
    return retVal;
}


void HXPthreadThread::_thread_exit(UINT32 unExitCode)
{
    pthread_exit( (void*)unExitCode );
}

void HXPthreadThread::_thread_cancel(ULONG32 ulThreadID)
{
    pthread_cancel( (pthread_t) ulThreadID );
}

ULONG32 HXPthreadThread::_thread_join(ULONG32 ulThreadID)
{
    void* pvRetVal = NULL;
    pthread_join( (pthread_t) ulThreadID, &pvRetVal );
    return (ULONG32)(PTR_INT)pvRetVal;
}




//=======================================================================
//
//                      HXPthreadMutex
//                   ------------------
//
//=======================================================================
HXPthreadMutex::HXPthreadMutex()
    : HXUnixMutex(),
      m_ulOwnerThread(0),
      m_ulLockCount(0)
{
     memset(&m_mutex,       0, sizeof(m_mutex));
     memset(&m_mtxLockLock, 0, sizeof(m_mtxLockLock));
#ifdef _TIMEDWAITS_RECURSIVE_MUTEXES
     pthread_mutexattr_t attr;
     pthread_mutexattr_init( &attr );
#ifdef PTHREAD_MUTEX_RECURSIVE_NP
     pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE_NP );
#else
     pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
#endif
     pthread_mutex_init( &m_mutex, &attr );
     pthread_mutexattr_destroy( &attr );
#else     
     pthread_mutex_init( &m_mutex,NULL);
     pthread_mutex_init( &m_mtxLockLock, NULL );
#endif     
}
    
HXPthreadMutex::~HXPthreadMutex()
{
    pthread_mutex_destroy( &m_mutex );
#ifndef _TIMEDWAITS_RECURSIVE_MUTEXES    
    m_ulLockCount   = 0;
    m_ulOwnerThread = 0;
    pthread_mutex_destroy( &m_mtxLockLock );
#endif    
}
    

HX_RESULT HXPthreadMutex::_Lock()
{
    //We simulate recursive mutexes.
    HX_RESULT res     = HXR_OK;

#ifndef _TIMEDWAITS_RECURSIVE_MUTEXES    
    int       nResult = 0;

    nResult = pthread_mutex_lock(&m_mtxLockLock);
    if (nResult != 0 )
    {
        res = HXR_FAIL;
        return res;
    }
        
    if( m_ulOwnerThread != pthread_self() )
    {
        pthread_mutex_unlock(&m_mtxLockLock);        
        //We are going to block for sure.
        nResult = pthread_mutex_lock(&m_mutex);

        //Take ownership.
        if ( pthread_mutex_lock(&m_mtxLockLock) != 0 )
        {
            //This should not happen but if does then unlock the
            //main mutex and return failure.
            if ( nResult == 0 )
            {
                pthread_mutex_unlock(&m_mutex);
            } 
            return HXR_FAIL;
        }
                
        if ( nResult == 0 )
        {
             m_ulOwnerThread = pthread_self();
             m_ulLockCount   = 1;
        }
        else
        {
            res = HXR_FAIL;
        }
    }
    else
    {
        //We alread have it locked. Just increment the lock count
        m_ulLockCount++;
    }
    pthread_mutex_unlock(&m_mtxLockLock);
#else
    pthread_mutex_lock(&m_mutex);
#endif    
    return res;
}
    
HX_RESULT HXPthreadMutex::_Unlock()
{
    HX_RESULT res     = HXR_OK;

#ifndef _TIMEDWAITS_RECURSIVE_MUTEXES    
    int       nResult = 0;

    nResult = pthread_mutex_lock(&m_mtxLockLock);
    if ( nResult != 0 )
    {
        res = HXR_FAIL;
        return res;
    }
    
    //Sanity checks.
    HX_ASSERT( m_ulLockCount != 0 && m_ulOwnerThread == pthread_self() );
    if( m_ulLockCount == 0 || m_ulOwnerThread!=pthread_self() )
    {
        pthread_mutex_unlock(&m_mtxLockLock);  
        return HXR_FAIL;
    }
    
    
    if( m_ulLockCount == 1 )
    {
        //We are really done with it. Do the real unlock now.
        nResult = pthread_mutex_unlock(&m_mutex);
        if ( nResult == 0 )
        {
            m_ulOwnerThread = 0;
            m_ulLockCount=0;
        }
        else
        {
            res = HXR_FAIL;
        }
    }
    else
    {
        m_ulLockCount--;
    }
    pthread_mutex_unlock(&m_mtxLockLock);
#else
    pthread_mutex_unlock(&m_mutex);
#endif    
    return res;
}
    
HX_RESULT HXPthreadMutex::_TryLock()
{
    HX_RESULT res = HXR_OK;    
    int       nResult = 0;

#ifndef _TIMEDWAITS_RECURSIVE_MUTEXES    
    nResult = pthread_mutex_lock(&m_mtxLockLock);
    if (nResult != 0 )
    {
        res = HXR_FAIL;
        return res;
    }
    
    if( m_ulOwnerThread != pthread_self() )
    {
        nResult = pthread_mutex_trylock(&m_mutex);
                
        if ( nResult == 0 )
        {
            m_ulOwnerThread = pthread_self();
            m_ulLockCount   = 1;
        }
        else
        {
            res = HXR_FAIL;
        }
    }
    else
    {
        //We alread have it locked. Just increment the lock count
        m_ulLockCount++;
    }
    pthread_mutex_unlock(&m_mtxLockLock);
#else
    nResult = pthread_mutex_trylock(&m_mutex);
    if ( nResult != 0 )
    {
        res = HXR_FAIL;
    }
#endif    
    return res;
}

pthread_mutex_t* HXPthreadMutex::_GetPthreadMutex()
{
    return &m_mutex; 
}

//=======================================================================
//
//                   HXPthreadCondition
//                   ----------------------
//
//=======================================================================
HXPthreadCondition::HXPthreadCondition(HXUnixMutex*& pMutex)
{
    HX_ASSERT( pMutex == NULL );
    //Create the mutex we need to associate with this cond.
    
    m_pMutex = new HXPthreadMutex();
    pMutex = (HXUnixMutex*)m_pMutex;

    //Init our cond var.
    pthread_cond_init( &m_cond, NULL );
}

HXPthreadCondition::~HXPthreadCondition()
{
    pthread_cond_destroy(&m_cond);
    HX_DELETE( m_pMutex );
}

HX_RESULT HXPthreadCondition::_Signal()
{
    pthread_cond_signal(&m_cond);
    return HXR_OK;
}

HX_RESULT HXPthreadCondition::_Broadcast()
{
    pthread_cond_broadcast(&m_cond);
    return HXR_OK;
}
HX_RESULT HXPthreadCondition::_Wait()
{
    HX_ASSERT( m_pMutex );
    //m_pMuex MUST BE LOCKED ALL READY!
    pthread_cond_wait(&m_cond, m_pMutex->_GetPthreadMutex());
    return HXR_OK;
}

HX_RESULT HXPthreadCondition::_TimedWait(UINT32 unTimeOut)
{
    //m_pMuex MUST BE LOCKED ALL READY!
    HX_RESULT       ret = HXR_OK;
    struct timeval  now;
    struct timespec timeout;
    int             retcode;

    gettimeofday(&now, NULL);
    long int waitSeconds = unTimeOut/1000;
    long int nanoSeconds = (unTimeOut-(waitSeconds*1000))*1000000;
    timeout.tv_sec  = now.tv_sec+waitSeconds;
    timeout.tv_nsec = now.tv_usec*1000+nanoSeconds;
    if( timeout.tv_nsec >= 1000000000 )
    {
        timeout.tv_nsec -= 1000000000;
        timeout.tv_sec  += 1;
    }

    retcode = pthread_cond_timedwait(&m_cond, m_pMutex->_GetPthreadMutex(), &timeout);
    
    if(retcode==-1)
    {
        ret = HXR_FAIL;
        //We really could use a HXR_TIMEDOUT.
        if( errno == ETIMEDOUT )
            ret = HXR_WOULD_BLOCK;
    }
    return ret;
}

#ifndef _MAC_UNIX

//=======================================================================
//
//                      HXPthreadSemaphore
//                   ------------------
//
//=======================================================================

HXPthreadSemaphore::HXPthreadSemaphore(UINT32 unInitialCount)
    : HXUnixSemaphore( unInitialCount )
{
    //Init the sem to non-shared and count passed in.

    if( sem_init( &m_semaphore, 0, m_unInitialCount ) < 0 )
    {
#ifdef _DEBUG
        fprintf( stderr, "Can't init semaphore: %d %s\n", errno, strerror(errno) );
#endif
    }
}

HXPthreadSemaphore::~HXPthreadSemaphore()
{
    sem_destroy( &m_semaphore );
}


HX_RESULT HXPthreadSemaphore::_Post()
{
    HX_RESULT retVal = HXR_OK;
    //Init the sem to non-shared and count passed in.
    if( sem_post(&m_semaphore) < 0 )
    {
#ifdef _DEBUG
        fprintf( stderr, "Can't post to semaphore: %d %s\n", errno, strerror(errno) );
#endif
        retVal = HXR_FAIL;
    }
    return retVal;
}

HX_RESULT HXPthreadSemaphore::_Wait()
{
    //sem_wait always returns zero.
    sem_wait( &m_semaphore );
    return HXR_OK;
}

HX_RESULT HXPthreadSemaphore::_TryWait()
{
    HX_RESULT retVal  = HXR_OK;
    int       nResult = 0;

    nResult = sem_trywait( &m_semaphore );
    if( nResult<0 && errno == EAGAIN )
    {
        retVal = HXR_WOULD_BLOCK;
    }
    else if( nResult < 0 )
    {
#ifdef _DEBUG
        fprintf( stderr, "Can't wait on semaphore: %d %s\n", errno, strerror(errno) );
#endif
        retVal = HXR_FAIL;
    }

    return retVal;
}
// #ifdef _TIMEDWAITS_RECURSIVE_MUTEXES    
// HX_RESULT HXPthreadSemaphore::_TimedWait(UINT32 unTimeOut )
// {
//     HX_RESULT ret = HXR_OK;
//     struct timeval  now;
//     struct timespec timeout;
//     int retcode;

//     gettimeofday(&now, NULL);
//     long int waitSeconds = unTimeOut/1000;
//     long int nanoSeconds = (unTimeOut-(waitSeconds*1000))*1000000;
//     if( nanoSeconds >= 1000000000 )
//     {
//         nanoSeconds -= 1000000000;
//         waitSeconds += 1;
//     }
    
//     timeout.tv_sec  = now.tv_sec+waitSeconds;
//     timeout.tv_nsec = now.tv_usec*1000+nanoSeconds;
//     if( timeout.tv_nsec >= 1000000000 )
//     {
//         timeout.tv_nsec -= 1000000000;
//         timeout.tv_sec  += 1;
//     }

//     //XXXgfw TEST TEST TEST
//     retcode = sem_timedwait(&m_semaphore, &timeout);
    
//     if(retcode==-1)
//     {
//         ret = HXR_FAIL;
//         //We really could use a HXR_TIMEDOUT.
//         if( errno == ETIMEDOUT )
//             ret = HXR_WOULD_BLOCK;
//     }
//     return ret;
// }
// #endif    

HX_RESULT HXPthreadSemaphore::_GetValue( int* pnCount)
{
    //sem_getvalue always returns zero.
    sem_getvalue( &m_semaphore, pnCount );
    return HXR_OK;
}


#else

// now  the _MAC_UNIX case...

//=======================================================================
//
//                      HXPthreadMacSemaphore
//                      ---------------------
//
//=======================================================================

HXPthreadMacSemaphore::HXPthreadMacSemaphore(UINT32 unInitialCount)
    : HXUnixSemaphore( unInitialCount )
{
    //Init the sem to non-shared and count passed in.

    char buf[32];
    sprintf(buf, "%s", tmpnam(NULL));
    sem_t* sem = sem_open(buf, O_CREAT, 0, m_unInitialCount);
    if ((int)sem == SEM_FAILED)
    {
#ifdef _DEBUG
        fprintf( stderr, "Can't open semaphore: %d %s\n", errno, strerror(errno) );
#endif
    }
    else
    {
        m_semaphore = sem;
    }
}

HXPthreadMacSemaphore::~HXPthreadMacSemaphore()
{
    if ( sem_close(m_semaphore) < 0 )
    {
#ifdef _DEBUG
        fprintf( stderr, "Can't close semaphore: %d %s\n", errno, strerror(errno) );
#endif
    }
}


HX_RESULT HXPthreadMacSemaphore::_Post()
{
    HX_RESULT retVal = HXR_OK;
    //Init the sem to non-shared and count passed in.
    if( sem_post(m_semaphore) < 0 )
    {
#ifdef _DEBUG
        fprintf( stderr, "Can't post to semaphore: %d %s\n", errno, strerror(errno) );
#endif
        retVal = HXR_FAIL;
    }
    return retVal;
}

HX_RESULT HXPthreadMacSemaphore::_Wait()
{
    //sem_wait always returns zero.
    if ( sem_wait( m_semaphore ) < 0)
    {
#ifdef _DEBUG
        fprintf( stderr, "sem_wait failed: %d %s\n", errno, strerror(errno) );
#endif
    }
    return HXR_OK;
}

HX_RESULT HXPthreadMacSemaphore::_TryWait()
{
    HX_RESULT retVal  = HXR_OK;
    int       nResult = 0;

    nResult = sem_trywait( m_semaphore );
    if( nResult<0 && errno == EAGAIN )
    {
        retVal = HXR_WOULD_BLOCK;
    }
    else if( nResult < 0 )
    {
#ifdef _DEBUG
        fprintf( stderr, "Can't wait on semaphore: %d %s\n", errno, strerror(errno) );
#endif
        retVal = HXR_FAIL;
    }

    return retVal;
}

HX_RESULT HXPthreadMacSemaphore::_GetValue( int* pnCount)
{
    //sem_getvalue always returns zero.
    if ( sem_getvalue( m_semaphore, pnCount ) < 0 )
    {
#ifdef _DEBUG
        fprintf( stderr, "sem_getvalue failed: %d %s\n", errno, strerror(errno) );
#endif
    }
    return HXR_OK;
}


#endif // _MAC_UNIX



#endif //_UNIX_THREADS_SUPPORTED
