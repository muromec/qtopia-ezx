/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pthreadthreads.h,v 1.10 2007/07/06 20:42:02 jfinnecy Exp $
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

#ifndef _PTHREADTHREADS
#define _PTHREADTHREADS

#include "UnixThreads.h"

class HXPthreadCondition;

//=======================================================================
//
//                      HXPthreadMutex
//                   ------------------
//
// NOTE: Mutexs must be recursive.
//=======================================================================
class HXPthreadMutex : public HXUnixMutex
{
public:
    //Ctors and dtors.
    HXPthreadMutex();
    virtual ~HXPthreadMutex();

    friend class HXPthreadCondition;
    
protected:
    //Must be recursive
    virtual HX_RESULT _Lock();
    virtual HX_RESULT _Unlock();
    virtual HX_RESULT _TryLock();

    //Used by freind class HXCondition for locking.
    pthread_mutex_t* _GetPthreadMutex();
        
private:

    pthread_mutex_t m_mutex;

    //These are used to simulate recursive mutexes on systems that don't
    //have them.
#ifndef _MAC_UNIX
    ULONG32         m_ulOwnerThread;
#else
    pthread_t       m_ulOwnerThread;
#endif
    ULONG32         m_ulLockCount;
    pthread_mutex_t m_mtxLockLock; //A mutex to protect the mutex!

    //Prevent unintentional ctors
    HXPthreadMutex( const HXPthreadMutex& ); //copy ctor
    HXPthreadMutex& operator=(const HXPthreadMutex& ); //assignment operator.
};



#ifndef _MAC_UNIX

typedef sem_t HXsem_t;

//=======================================================================
//
//                   HXPthreadSemaphore
//                   ------------------
//
//=======================================================================
class HXPthreadSemaphore : public HXUnixSemaphore
{
  public:
    //Ctors and dtors.
    HXPthreadSemaphore(UINT32 unInitialCount=0);
    virtual ~HXPthreadSemaphore();
        
protected:

    virtual HX_RESULT _Post();
    virtual HX_RESULT _Wait();
    virtual HX_RESULT _TryWait();
    virtual HX_RESULT _GetValue(int* pnCount );

private:

    HXsem_t m_semaphore;
    
    //Prevent unintentional ctors
    HXPthreadSemaphore( const HXPthreadSemaphore& ); //copy ctor
    HXPthreadSemaphore& operator=(const HXPthreadSemaphore& ); //assignment operator.
};

#else

//=======================================================================
//
//                   HXPthreadmacSemaphore
//                   ---------------------
//
//=======================================================================
class HXPthreadMacSemaphore : public HXUnixSemaphore
{
  public:
    //Ctors and dtors.
    HXPthreadMacSemaphore(UINT32 unInitialCount=0);
    virtual ~HXPthreadMacSemaphore();
        
protected:

    virtual HX_RESULT _Post();
    virtual HX_RESULT _Wait();
    virtual HX_RESULT _TryWait();
    virtual HX_RESULT _GetValue(int* pnCount );

private:

    sem_t* m_semaphore;
    
    //Prevent unintentional ctors
    HXPthreadMacSemaphore( const HXPthreadMacSemaphore& ); //copy ctor
    HXPthreadMacSemaphore& operator=(const HXPthreadMacSemaphore& ); //assignment operator.
};

#endif

//=======================================================================
//
//                   HXPthreadCondition
//                   ----------------------
//
//=======================================================================
class HXPthreadCondition : public HXUnixCondition
{
public:
    
    HXPthreadCondition(HXUnixMutex*& pMutex);
    virtual ~HXPthreadCondition();

protected:
    
    virtual HX_RESULT _Wait();
    virtual HX_RESULT _TimedWait( UINT32 uTimeoutPeriod = ALLFS);
    virtual HX_RESULT _Broadcast();
    virtual HX_RESULT _Signal();

private:

    HXPthreadMutex* m_pMutex;
    pthread_cond_t   m_cond;

    //Protect unintentional ctors
    HXPthreadCondition( const HXPthreadCondition& ); //copy ctor.
    HXPthreadCondition& operator=(const HXPthreadCondition& ); //assignment oper.
};



//=======================================================================
//
//                    HXPthreadThread    
//                   ------------------
//
//=======================================================================
class HXPthreadThread : public HXUnixThread
{
  public:
    HXPthreadThread();
    virtual ~HXPthreadThread();

protected:

    //
    // Pure virtuals that implement the OS specific chores.
    //
    virtual HX_RESULT _thread_create( ULONG32& ulThreadID, void*(pfExecFunc(void*)), void* pArg );
    virtual void      _thread_exit(UINT32 unExitCode);
    virtual void      _thread_cancel(ULONG32 ulThreadID);
    virtual ULONG32   _thread_join(ULONG32 ulThreadID);
    
private:

    //Prevent unintentional ctors
    HXPthreadThread( const HXPthreadThread& ); //copy ctor
    HXPthreadThread& operator=(const HXPthreadThread&); //assignment operator.
};


#endif /*_PTHREADTHREADS */
