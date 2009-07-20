/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: solaristhreads.cpp,v 1.6 2004/10/27 23:41:10 liam_murray Exp $
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

#include <synch.h>
#include <thread.h>
#include "solaristhreads.h"

ULONG32 HXUnixThread::GetCurrentThreadID()
{
    return thr_self();
}

//=======================================================================
//
//                      HXSolarisThread
//                   ----------------------
//
//=======================================================================
HXSolarisThread::HXSolarisThread()
    : HXUnixThread()
{}

HXSolarisThread::~HXSolarisThread()
{}

HX_RESULT HXSolarisThread::_thread_create( ULONG32& ulThreadID, void*(pfExecFunc(void*)), void* pArg )
{
    HX_RESULT retVal = HXR_OK;
    thread_t threadID=0;
    int nCode = thr_create(NULL, 0, pfExecFunc, pArg, 0, &threadID);
    ulThreadID = threadID;
    if (nCode!=0)
    {
        ulThreadID = 0;
        retVal = HXR_FAIL;
    }
    return retVal;
}

void HXSolarisThread::_thread_exit(UINT32 unExitCode)
{
    thr_exit( (void*)unExitCode );
}

void HXSolarisThread::_thread_cancel(ULONG32 ulThreadID)
{
    HX_ASSERT( "_thread_cancel not supported on Solaris threads" == NULL );
}

ULONG32 HXSolarisThread::_thread_join(ULONG32 ulThreadID)
{
    void* pvRetVal = NULL;
    thr_join( ulThreadID, NULL, &pvRetVal );
    return (ULONG32)(PTR_INT)pvRetVal;
}




//=======================================================================
//
//                      HXSolarisMutex
//                   ------------------
//
//=======================================================================
HXSolarisMutex::HXSolarisMutex()
    : HXUnixMutex(),
      m_ulOwnerThread(0),
      m_ulLockCount(0)
{
    mutex_init( &m_mutex, USYNC_THREAD, NULL ); 
    mutex_init( &m_mtxLockLock, USYNC_THREAD, NULL );
}
    
HXSolarisMutex::~HXSolarisMutex()
{
    mutex_destroy( &m_mutex );
    m_ulLockCount   = 0;
    m_ulOwnerThread = 0;
}
    

HX_RESULT HXSolarisMutex::_Lock()
{
    //We simulate recursive mutexes.

    //No one owns this mutex.

    if( m_ulOwnerThread != thr_self() )
    {
        //We are going to block for sure.
        mutex_lock(&m_mutex);

        //Take ownership.
        mutex_lock(&m_mtxLockLock);
        m_ulOwnerThread = thr_self();
        m_ulLockCount   = 1;
        mutex_unlock(&m_mtxLockLock);        
    }
    else
    {
        //We alread have it locked. Just increment the lock count
        mutex_lock(&m_mtxLockLock);
        m_ulLockCount++;
        mutex_unlock(&m_mtxLockLock);        
    }

    return HXR_OK;
}
    
HX_RESULT HXSolarisMutex::_Unlock()
{
    //Sanity checks.
    HX_ASSERT( m_ulLockCount != 0 && m_ulOwnerThread == thr_self() );
    if( m_ulLockCount == 0 || m_ulOwnerThread!=thr_self() )
        return HXR_FAIL;

    mutex_lock(&m_mtxLockLock);
    m_ulLockCount--;
    if( m_ulLockCount == 0 )
    {
        //We are really done with it. Do the real unlock now.
        m_ulOwnerThread = 0;
        mutex_unlock(&m_mutex);
    }

    mutex_unlock(&m_mtxLockLock);  
    return HXR_OK;
}
    
HX_RESULT HXSolarisMutex::_TryLock()
{
//    HX_ASSERT( "Trylock isn't compatible with our home-grown recursive mutexes yet!" ==NULL );
    //Warning: this is just a blind wrapper around trylock. It doesn't take into accout
    //our simulation or recursive mutexes.
    return (mutex_trylock(&m_mutex)==0) ? HXR_OK : HXR_FAIL;
}



//=======================================================================
//
//                      HXSolarisSemaphore
//                   ------------------
//
//=======================================================================

HXSolarisSemaphore::HXSolarisSemaphore(UINT32 unInitialCount)
    : HXUnixSemaphore( unInitialCount )
{
    //Init the sem to non-shared and count passed in.
    if( sema_init( &m_semaphore, m_unInitialCount, USYNC_THREAD, NULL ) < 0 )
    {
#ifdef _DEBUG
        fprintf( stderr, "Can't init semaphore: %d %s\n", errno, strerror(errno) );
#endif
    }
}

HXSolarisSemaphore::~HXSolarisSemaphore()
{
    sema_destroy( &m_semaphore );
}


HX_RESULT HXSolarisSemaphore::_Post()
{
    HX_RESULT retVal = HXR_OK;
    //Init the sem to non-shared and count passed in.
    if( sema_post(&m_semaphore) < 0 )
    {
#ifdef _DEBUG
        fprintf( stderr, "Can't post to semaphore: %d %s\n", errno, strerror(errno) );
#endif
        retVal = HXR_FAIL;
    }
    return retVal;
}

HX_RESULT HXSolarisSemaphore::_Wait()
{
    //sem_wait always returns zero.
    sema_wait( &m_semaphore );
    return HXR_OK;
}

HX_RESULT HXSolarisSemaphore::_TryWait()
{
    HX_RESULT retVal  = HXR_OK;
    int       nResult = 0;
    
    nResult = sema_trywait( &m_semaphore );
    if( nResult != 0 )
    {
        retVal = HXR_WOULD_BLOCK;
    }

    return retVal;
}

HX_RESULT HXSolarisSemaphore::_GetValue( int* pnCount)
{
    //sem_getvalue always returns zero.
    HX_ASSERT( "_GetValue is not supported by Solaris threads"==NULL );
    
    return HXR_OK;
}



HXSolarisCondition::HXSolarisCondition(HXUnixMutex*& pMutex)
{
    HX_ASSERT( pMutex == NULL );
    //Create the mutex we need to associate with this cond.
    
    m_pMutex = new HXSolarisMutex();
    pMutex = (HXUnixMutex*)m_pMutex;

    //Init our cond var.
    cond_init( &m_cond, NULL, NULL );
}

HXSolarisCondition::~HXSolarisCondition()
{
    cond_destroy(&m_cond);
    HX_DELETE( m_pMutex );
}

HX_RESULT HXSolarisCondition::_Signal()
{
    cond_signal(&m_cond);
    return HXR_OK;
}

HX_RESULT HXSolarisCondition::_Broadcast()
{
    cond_broadcast(&m_cond);
    return HXR_OK;
}
HX_RESULT HXSolarisCondition::_Wait()
{
    HX_ASSERT( m_pMutex );
    //m_pMuex MUST BE LOCKED ALL READY!
    cond_wait(&m_cond, m_pMutex->_GetSolarisMutex());
    return HXR_OK;
}

HX_RESULT HXSolarisCondition::_TimedWait(UINT32 unTimeOut)
{
    //m_pMuex MUST BE LOCKED ALL READY!
    HX_RESULT      ret = HXR_OK;
    struct timeval now;
    timestruc_t    timeout;
    int            retcode;

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

    retcode = cond_timedwait(&m_cond, m_pMutex->_GetSolarisMutex(), &timeout);
    
    if(retcode==-1)
    {
        ret = HXR_FAIL;
        //We really could use a HXR_TIMEDOUT.
        if( errno == ETIMEDOUT )
            ret = HXR_WOULD_BLOCK;
    }
    return ret;
}



#endif //_UNIX_THREADS_SUPPORTED
