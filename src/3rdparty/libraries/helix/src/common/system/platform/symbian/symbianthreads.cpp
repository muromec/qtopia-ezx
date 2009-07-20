/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbianthreads.cpp,v 1.22 2009/04/14 13:40:16 amsaleem Exp $
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
//This is used to turn off threads in libc5 builds.
#include "hxtypes.h"  //for UINT32
#include "hxresult.h" //for HX_RESULT
#include "hxslist.h"
#include "hxassert.h" //for HX_ASSERT
#include "hxmsgs.h"   //for HXMSG_ASYNC_TIMER message.
#include "hxmap.h"    //for CHXMapLongToObj
#include "hxtick.h"   //for GetTickCount()
#include "globals/hxglobals.h"


#include "SymbianThreads.h"

#include "symbian_async_timer_imp.h"
#include "symbian_rtimer_at_imp.h"
#include "symbian_thread_at_imp.h"
#include "hxglobalmgr_inst.h"
#include "pckunpck.h"

static const IHXMutex* const z_pMapLock = NULL; // Used by HXSymbianAsyncTimer

//XXXgfw I first coded this to use RTimers for RunL callbacks to get
//the work of the async timer done. But, if the thread that creates
//the timer blocks at all we stop getting callbacks. I think now that
//this may cause problems with the way Helix uses the timers. So, I am
//going to code a version that uses real threads to get the work
//done. You can use the below define to build either one you want.
//#define  USE_SYMBIAN_THREADS_FOR_ASYNCTIMERS

//=======================================================================
//
//                      HXSymbianThread
//                   ------------------
//
//=======================================================================
HXSymbianThread::HXSymbianThread()
    : m_pThread(NULL),
      m_pmtxQue(NULL),
      m_pSchedulerWait(NULL),
      m_pActiveObj(NULL)
{
    HXMutex::MakeMutex( m_pmtxQue );
    HX_ASSERT( m_pmtxQue );
}

HXSymbianThread::~HXSymbianThread()
{
    //Clean up message que.
    while( !m_messageQue.IsEmpty() )
    {
        HXThreadMessage* pTmp = (HXThreadMessage *)(m_messageQue.RemoveHead());
        HX_DELETE( pTmp );
    }

    HX_DELETE( m_pmtxQue );

    if( m_pThread )
    {
        m_pThread->Kill(0);
        m_pThread->Close();
        HX_DELETE(m_pThread);
    }
}

HX_RESULT HXSymbianThread::CreateThread( void*(pfExecFunc(void*)),
                                         void* pArg,
                                         ULONG32 ulCreationFlags)
{
    HX_RESULT retVal = HXR_OK;
    TInt      err    = KErrNone;

    HX_ASSERT( NULL==m_pThread );
    if( NULL != m_pThread )
    {
        retVal = HXR_UNEXPECTED;
    }
    else
    {
        m_pThread = new RThread();
        if( NULL == m_pThread )
        {
            retVal = HXR_FAIL;
        }
        else
        {
            //Each thread has to have a unique name. Using RThread ptr to generate it.
            _LIT(KThreadBaseName, "HXThread");
            TBuf<50> ThreadName(KThreadBaseName);
            _LIT(KHexFormat, "_0x%x");
            ThreadName.AppendFormat(KHexFormat, m_pThread);

            st_execStruct* pstExecStruct = new st_execStruct;
            pstExecStruct->pfExecProc     = (TThreadFunction)pfExecFunc;
            pstExecStruct->pExecArg       = pArg;
            pstExecStruct->pGlobalManager = HXGlobalManInstance::GetInstance();
            pstExecStruct->pOwner         = this;

            err = m_pThread->Create( ThreadName,
                                     _ThreadWrapper,
                                     KDefaultStackSize,
                                     &User::Heap(), //Use parent HEAP
                                     pstExecStruct
                                     );
            if( KErrNone != err )
            {
                //Could not start thread.
                retVal = HXR_FAIL;
                m_pThread->Close();
                HX_DELETE( m_pThread );
                HX_DELETE(pstExecStruct);
            }
            else
            {
                //Tell the system we want to be notifed when this
                //thread terminates for any reason.
                m_pThread->Logon(m_reqStat);

                //Symbian threads start up in a 'suspended' mode. HXThreads
                //assume that the threads are running from the get go. So, we
                //do a resume here.
                m_pThread->Resume();
            }
        }
    }

    return retVal;
}


HX_RESULT HXSymbianThread::Exit(UINT32 unExitCode)
{
    if( NULL==m_pThread )
    {
        //Thread has already gone or was never here.
        return HXR_UNEXPECTED;
    }

    if( m_pThread->Id() != (TThreadId)GetCurrentThreadID() )
    {
        //Ok, because of the way winthrd.cpp does it, this call also
        //acts as a 'pthread_join' when the calling thread isn't the
        //m_threadID.

        //Also, it looks like this method isn't set up to look at the
        //return value of the thread. We could return HXR_FAIL is it
        //is anything except 0 but for now just throw it away.
        User::WaitForRequest(m_reqStat);
    }
    else
    {
        //Kill our selves....
        m_pThread->Kill(unExitCode);
    }

    return HXR_OK;
}


HX_RESULT HXSymbianThread::SetPriority( UINT32 ulPriority)
{
    HX_RESULT res = HXR_FAIL;
    if( m_pThread)
    {
        m_pThread->SetPriority((TThreadPriority)ulPriority);
        res = HXR_OK;
    }
    return res;
}

HX_RESULT HXSymbianThread::GetPriority( UINT32& ulPriority)
{
    HX_RESULT res = HXR_FAIL;
    if( m_pThread)
    {
        ulPriority = m_pThread->Priority();
        res = HXR_OK;
    }
    return res;
}

HX_RESULT HXSymbianThread::YieldTimeSlice()
{
    //Just sleep for a tiny amount so someone else can
    //run.
    User::After(10);
    return HXR_OK;
}


HX_RESULT HXSymbianThread::GetThreadId(UINT32& ulThreadId)
{
    HX_RESULT res = HXR_FAIL;
    if( m_pThread )
    {
        ulThreadId = m_pThread->Id();
        res = HXR_OK;
    }

    return res;
}

ULONG32 HXSymbianThread::GetCurrentThreadID()
{
    return RThread().Id();
}

HX_RESULT HXSymbianThread::Suspend()
{
    HX_RESULT res = HXR_FAIL;
    if( m_pThread )
    {
        m_pThread->Suspend();
        res = HXR_OK;
    }

    return res;
}

HX_RESULT HXSymbianThread::Resume()
{
    HX_RESULT res = HXR_FAIL;
    if( m_pThread )
    {
        m_pThread->Resume();
        res = HXR_OK;
    }

    return res;
}

HX_RESULT HXSymbianThread::PostMessage(HXThreadMessage* pMsg, void* pWindowHandle)
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
            m_pmtxQue->Lock();

            m_messageQue.AddTail((void*)pMsgTmp);

            //If we were empty the the GetMessage thread could have
            //been waiting on us to post. Signal it.
            if( m_pThread && m_pActiveObj->IsActive())
            {
               TRequestStatus *pReqStatus = &(m_pActiveObj->Status());
               m_pThread->RequestComplete(pReqStatus, KErrNone);
            }
            m_pmtxQue->Unlock();
        }
    }

    return retVal;
}

HX_RESULT HXSymbianThread::GetMessage( HXThreadMessage* pMsg,
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
        m_pmtxQue->Lock();
        if (m_messageQue.IsEmpty())
        {
            m_pActiveObj->Status() = KRequestPending;
            m_pActiveObj->Activate();
            m_pmtxQue->Unlock();
            m_pSchedulerWait->Start();
            m_pmtxQue->Lock();
        }
        //Sanity check.
        HX_ASSERT( !m_messageQue.IsEmpty() );

        HXThreadMessage* pMsgTmp = (HXThreadMessage*)m_messageQue.RemoveHead();
        if( pMsgTmp != NULL )
        {
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
        m_pmtxQue->Unlock();
    }

    return retVal;
}
HX_RESULT HXSymbianThread::PeekMessageMatching( HXThreadMessage* pMsg,
                                              HXThreadMessage* pMatch,
                                              HXBOOL bRemoveMessage )
{
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT( pMsg );
    HX_ASSERT( pMatch );

    if( pMsg != NULL && pMatch!=NULL )
    {
        //Protect the que.
        m_pmtxQue->Lock();

        //Sanity check.
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
        m_pmtxQue->Unlock();
    }
    return retVal;
}

HX_RESULT HXSymbianThread::PeekMessage( HXThreadMessage* pMsg,
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
        m_pmtxQue->Lock();

        //Sanity check.
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
        m_pmtxQue->Unlock();
    }
    return retVal;
}
HX_RESULT HXSymbianThread::DispatchMessage(HXThreadMessage* pMsg)
{
    HX_ASSERT( "HXSymbianThread::DispatchMessage is not implemented yet." == NULL );
    return HXR_FAIL;
}

void HXSymbianThread::ProcessRunL(TRequestStatus aStatus, TAny* pData)
{
    if (m_pSchedulerWait->IsStarted())
    {
       m_pSchedulerWait->AsyncStop();
    }
}

void HXSymbianThread::ProcessDoCancel(TAny* pData)
{
    m_pActiveObj->Status() = KErrCancel;
}

TInt HXSymbianThread::_ThreadWrapper(TAny* pExecStruct)
{
    TInt nRetVal = KErrNone;

    st_execStruct* pstExec = (st_execStruct*)pExecStruct;

    //Intall a cleanup trap
    CTrapCleanup* pTrapCleanup = CTrapCleanup::New();

    //Install a handle to the global manager for this thread.
    HXGlobalManInstance::SetInstance(pstExec->pGlobalManager);

    //Install an active Scheduler for this thread.
    CActiveScheduler* pSched = new CActiveScheduler();
    CActiveScheduler::Install(pSched);

    pstExec->pOwner->m_pSchedulerWait = new CActiveSchedulerWait;
    pstExec->pOwner->m_pActiveObj = new CActiveObj(pstExec->pOwner);

    //Call the thread.
    TRAP(nRetVal, nRetVal = pstExec->pfExecProc(pstExec->pExecArg));

	HX_DELETE(pstExec->pOwner->m_pActiveObj);
    HX_DELETE(pstExec->pOwner->m_pSchedulerWait);

    CActiveScheduler::Install(0);
    HX_DELETE(pSched);
    HX_DELETE(pstExec);
    HX_DELETE(pTrapCleanup);

    return nRetVal;
}



//=======================================================================
//
//                      HXSymbianMutex
//                   ------------------
//
//=======================================================================
HXSymbianMutex::HXSymbianMutex()
    : m_pCritSec(NULL),
      m_bInited(FALSE),
      m_pCritSecLck(NULL),
      m_ulOwnerThread(0),
      m_ulLockCount(0)
{
}

HXSymbianMutex::~HXSymbianMutex()
{
    if( m_pCritSec )
    {
        m_pCritSec->Close();
        HX_DELETE(m_pCritSec);
    }
    if( m_pCritSecLck )
    {
        m_pCritSecLck->Close();
        HX_DELETE(m_pCritSecLck);
    }
    m_bInited = FALSE;
}

HX_RESULT HXSymbianMutex::_Init()
{
    TInt      err1 = KErrNone;
    TInt      err2 = KErrNone;
    HX_RESULT res  = HXR_FAIL;

    if( !m_bInited)
    {
        m_pCritSec = new RCriticalSection;
        if( m_pCritSec )
        {
            err1 = m_pCritSec->CreateLocal();
        }
        m_pCritSecLck = new RCriticalSection;
        if( m_pCritSecLck )
        {
            err2 = m_pCritSecLck->CreateLocal();
        }
    }
    if( m_pCritSec && m_pCritSecLck && KErrNone==err1 && KErrNone==err2 )
    {
        res = HXR_OK;
        m_bInited = TRUE;
    }

    return res;
}


HX_RESULT HXSymbianMutex::Lock()
{
    HX_RESULT res = HXR_OK;
    RThread   me;

    //XXXgfw I wonder if the check every time is too much of a perf
    //hit.
    if( !m_bInited )
    {
        res = _Init();
        if( FAILED(res) )
            return res;
    }

    m_pCritSecLck->Wait();
    if( m_ulOwnerThread != me.Id() )
    {
        m_pCritSecLck->Signal();
        m_pCritSec->Wait();
        m_pCritSecLck->Wait();
        m_ulOwnerThread = me.Id();
        //Make sure the lock count is always zero when we hand off a
        //mutex to another thread. Otherwise there was too many
        //unlocks or the like.
        HX_ASSERT( m_ulLockCount == 0 );
        m_ulLockCount   = 1;
    }
    else
    {
        //We alread have it locked. Just increment the lock count
        m_ulLockCount++;
    }
    m_pCritSecLck->Signal();

    return res;
}

HX_RESULT HXSymbianMutex::Unlock()
{
    HX_RESULT res = HXR_OK;
    RThread me;

    //XXXgfw I wonder if the check every time is too much of a perf
    //hit.
    if( !m_bInited )
    {
        res = _Init();
        if( FAILED(res) )
            return res;
    }


    m_pCritSecLck->Wait();

    HX_ASSERT( m_ulLockCount != 0 && m_ulOwnerThread == me.Id() );
    if( m_ulLockCount == 0 || m_ulOwnerThread!=me.Id() )
    {
        m_pCritSecLck->Signal();
        return HXR_FAIL;
    }

    if( m_ulLockCount == 1 )
    {
        //We are really done with it. Do the real unlock now.
        m_pCritSec->Signal();
        m_ulOwnerThread = 0;
        m_ulLockCount=0;
    }
    else
    {
        m_ulLockCount--;
    }
    m_pCritSecLck->Signal();

    return res;
}

HX_RESULT HXSymbianMutex::Trylock()
{
    HX_RESULT res = HXR_OK;
    RThread me;

    //XXXgfw I wonder if the check every time is too much of a perf
    //hit.
    if( !m_bInited )
    {
        res = _Init();
        if( FAILED(res) )
            return res;
    }

    m_pCritSecLck->Wait();
    if( m_pCritSec->IsBlocked() )
    {
        //Someone has it.
        HX_ASSERT( m_ulOwnerThread != 0 );
        HX_ASSERT( m_ulLockCount != 0 );
        if( m_ulOwnerThread == me.Id() )
        {
            //It us.
            m_ulLockCount++;
            res = HXR_OK;
        }
        else
        {
            //We would block on it.
            res = HXR_FAIL;
        }
    }
    else
    {
        //No one has it. grab it now.
        HX_ASSERT( m_ulOwnerThread == 0 );
        HX_ASSERT( m_ulLockCount == 0 );
        m_pCritSec->Wait();
        m_ulLockCount=1;
        m_ulOwnerThread = me.Id();
        res = HXR_OK;
    }

    m_pCritSecLck->Signal();

    return res;
}

//=======================================================================
//
//                  HXSymbianSemaphore
//                  ------------------
// NOTE:- As m_nWaitCount is not protected with a lock, instance of this class
//        has to be protected with a lock.
//
//=======================================================================
HXSymbianSemaphore::HXSymbianSemaphore()
: m_nCount(0)
, m_pCondLock(NULL)
{
    m_pSemaphore = new RSemaphore();
    HX_ASSERT(m_pSemaphore);

    HXMutex* pTmp =  NULL;
    HXMutex::MakeMutex(pTmp);
    m_pCondLock = (HXSymbianMutex*)pTmp;
    HX_ASSERT(m_pCondLock);
}

HXSymbianSemaphore::~HXSymbianSemaphore()
{
    m_pCondLock->Lock();
    HX_DELETE(m_pSemaphore);
    HX_DELETE(m_pCondLock);
}

INT32 HXSymbianSemaphore::CreateLocal(INT32 aCount,TOwnerType aType)
{
    m_pCondLock->Lock();
    m_nCount = aCount;
    INT32 nRet = m_pSemaphore->CreateLocal(aCount, aType);
    m_pCondLock->Unlock();
    return nRet;
}

void HXSymbianSemaphore::Wait()
{
    m_pCondLock->Lock();
    m_nCount--;
    if (m_nCount < 0)
    {
        m_pCondLock->Unlock();
        m_pSemaphore->Wait();
    }
    else
    {
        m_pCondLock->Unlock();
    }
}

void HXSymbianSemaphore::Signal(UINT32 uCount)
{
    m_pCondLock->Lock();
    HX_ASSERT(uCount > 0);
    while(uCount > 0)
    {
        m_pSemaphore->Signal();
        uCount--;
        m_nCount++;
    }
    m_pCondLock->Unlock();
}

INT32 HXSymbianSemaphore::GetCount() const
{
    m_pCondLock->Lock();
    INT32 uCount = m_nCount;
    m_pCondLock->Unlock();
    return uCount;
}

void HXSymbianSemaphore::Close()
{
    m_pSemaphore->Close();
}


//=======================================================================
//
//                     HXSymbianEvent
//                  ------------------
//
//=======================================================================
HXSymbianEvent::HXSymbianEvent(const char* pEventName, HXBOOL bManualReset)
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
    m_pCond = new HXSymbianSemaphore();
    if( m_pCond )
    {
        m_pCond->CreateLocal(0);
    }

    HXMutex* pTmp =  NULL;
    HXMutex::MakeMutex(pTmp);
    m_pCondLock = (HXSymbianMutex*)pTmp;
    HX_ASSERT( m_pCondLock && m_pCond );

    //We ignore the EventName for now...
}

HXSymbianEvent::~HXSymbianEvent()
{
    if( m_pCond )
    {
		m_pCond->Close();
        HX_DELETE( m_pCond );
    }
    HX_DELETE(m_pCondLock);
}


HX_RESULT HXSymbianEvent::Wait( UINT32 uTimeoutPeriod )
{
    HX_RESULT res = HXR_OK;

    //Symbian does not support timed waits...
    HX_ASSERT( uTimeoutPeriod==ALLFS);

    m_pCondLock->Lock();

    //Check to see if this event has already been signaled.
    if( !m_bEventIsSet )
    {
        //We must wait on the event then....
        m_pCondLock->Unlock();
        m_pCond->Wait();
        m_pCondLock->Lock();
    }

    //If we are not manual reset and we are signaled. reset the
    //signaled flag before returning otherwise leave it signaled for
    //everyone else that comes in to wait.
    if( !m_bIsManualReset )
    {
        m_bEventIsSet = FALSE;
    }

    //Now that we have waited
    m_pCondLock->Unlock();
    return res;
}

HX_RESULT HXSymbianEvent::ResetEvent()
{
    m_pCondLock->Lock();
    m_bEventIsSet = FALSE;
    m_pCondLock->Unlock();
    return HXR_OK;
}

HX_RESULT HXSymbianEvent::SignalEvent()
{
    m_pCondLock->Lock();

    m_bEventIsSet = TRUE;

   //There are one or more waiters...
   if (m_bIsManualReset)
   {
       m_pCond->Signal(Abs(m_pCond->GetCount()));
   }
   else
   {
       m_pCond->Signal();
   }
   m_pCondLock->Unlock();
   return HXR_OK;
}



void* HXSymbianEvent::GetEventHandle()
{
    return (void*)m_pCond;
}




//=======================================================================
//
//                     HXSymbianAsyncTimer
//                  -----------------------
//
//=======================================================================

static void DestroyMapLock(GlobalType pObj)
{
    IHXMutex* pMapLock = (IHXMutex*)pObj;
    HX_RELEASE(pMapLock);
}

UINT32 HXSymbianAsyncTimer::SetTimer(IUnknown* pContext, ULONG32 ulTimeOut, IHXThread* pReceivingThread )
{
    HXGlobalManager* pGM = HXGlobalManager::Instance();

    GlobalPtr ptr = pGM->Get(&z_pMapLock);

    if (!ptr)
    {
        IHXMutex* pMapLock = 0;
        CreateInstanceCCF(CLSID_IHXMutex, (void**)&pMapLock, pContext);  

        ptr = pGM->Add(&z_pMapLock, pMapLock, &DestroyMapLock);
    }
    
	//lock it.
	HX_LOCK((IHXMutex*)(*ptr));
	
    ULONG32 ulTimerID = 0;

    HX_ASSERT( ulTimeOut != 0 );
    HX_ASSERT( pReceivingThread != NULL );

    HXSymbianAsyncTimerImp* pTimer = CreateTimer();

    HX_ASSERT( pTimer != NULL );
    if( pTimer != NULL )
    {
        //Make sure the timeout is an even multiple of our
        //system tick. 15ms for Symbian phones we have ported
        //to so far. XXXgfw get this from a Symbian call.
        ulTimeOut = ulTimeOut/15*15;
        pTimer->Init(ulTimeOut, pReceivingThread );

        //Add new timer to map.
        ulTimerID = pTimer->GetID();
        GetMapTimers().SetAt( ulTimerID, (void*)pTimer );
    }

    //unlock the map.
    HX_UNLOCK((IHXMutex*)(*ptr));

    return ulTimerID;
}

UINT32 HXSymbianAsyncTimer::SetTimer(IUnknown* pContext, ULONG32 ulTimeOut, TIMERPROC pfExecFunc )
{
	HXGlobalManager* pGM = HXGlobalManager::Instance();

    GlobalPtr ptr = pGM->Get(&z_pMapLock);

    if (!ptr)
    {
        IHXMutex* pMapLock = 0;
        CreateInstanceCCF(CLSID_IHXMutex, (void**)&pMapLock, pContext);  

        ptr = pGM->Add(&z_pMapLock, pMapLock, &DestroyMapLock);
    }
    
	//lock it.
	HX_LOCK((IHXMutex*)(*ptr));

    ULONG32 ulTimerID = 0;

    HX_ASSERT( ulTimeOut != 0 );
    HX_ASSERT( pfExecFunc != NULL );

    HXSymbianAsyncTimerImp* pTimer = CreateTimer();

    HX_ASSERT( pTimer != NULL );
    if( pTimer != NULL )
    {
        ulTimeOut = ulTimeOut/15*15;
        pTimer->Init(ulTimeOut, pfExecFunc );

        //Add new timer to map.
        ulTimerID = pTimer->GetID();
        GetMapTimers().SetAt( ulTimerID, (void*)pTimer );
    }

    //unlock the map.
    HX_UNLOCK((IHXMutex*)(*ptr));

    return ulTimerID;
}

HXBOOL HXSymbianAsyncTimer::KillTimer(UINT32 ulTimerID )
{
    HXGlobalManager* pGM = HXGlobalManager::Instance();
    GlobalPtr ptr = pGM->Get(&z_pMapLock);

	//lock it.
	HX_LOCK((IHXMutex*)(*ptr));

    HXBOOL  bRetVal = FALSE;
    void* pTimer  = NULL;

    HX_ASSERT( ulTimerID != 0 );

    if( GetMapTimers().Lookup( ulTimerID, pTimer ) )
    {
        //Found it.
        HXSymbianAsyncTimerImp* pTmp = (HXSymbianAsyncTimerImp*)pTimer;
        bRetVal = TRUE;

        // delete is not used because the timer
        // may need to delay it's destruction.
        // The Destroy() call hides this detail
        pTmp->Destroy();
        pTmp = 0;

        GetMapTimers().RemoveKey( ulTimerID );
    }

    //unlock the map.
     HX_UNLOCK((IHXMutex*)(*ptr));

    return bRetVal;
}

HXSymbianAsyncTimerImp* HXSymbianAsyncTimer::CreateTimer()
{
    HXSymbianAsyncTimerImp* pRet = 0;

#if defined USE_SYMBIAN_THREADS_FOR_ASYNCTIMERS
    pRet = new HXSymbianThreadAsyncTimer;
#else
    pRet = new HXSymbianRTimerAsyncTimer;
#endif

    return pRet;
}

CHXMapLongToObj& HXSymbianAsyncTimer::GetMapTimers()
{
    static const CHXMapLongToObj* const z_pMapTimers = NULL;

    return HXGlobalMapLongToObj::Get(&z_pMapTimers);
}
