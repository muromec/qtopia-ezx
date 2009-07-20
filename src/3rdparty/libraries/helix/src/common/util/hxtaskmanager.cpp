/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 



#include "hxtypes.h"
#include "hxcom.h"
#include "hxslist.h"
#include "hxthread.h"
#include "hxscope_lock.h"

#include "hxmap.h"
#include "debug.h"
#include "hxassert.h"
#include "hxheap.h"
#include "hxtlogutil.h"
#include "hxtaskmanager.h"
#include "pckunpck.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif									 


HXTaskManager::Task::Task()
: m_pResponseThread(NULL)
, m_pMsgSink(NULL)
{
}


HXTaskManager::Task::~Task()
{
    HX_RELEASE(m_pResponseThread);
    HXRC_RELEASE(m_pMsgSink);
}


IHXThread* HXTaskManager::Task::GetResponseThread()
{
#if defined(_UNIX)
    if(!m_pResponseThread)
    {
    	if(m_pMsgSink)
    	{
    		m_pResponseThread = (IHXThread*)m_pMsgSink->GetThread();
    		m_pResponseThread->AddRef();
    	}
    }
#endif	
    return m_pResponseThread;
}

void HXTaskManager::Task::SetResponseThread(IHXThread* pThread)
{
    HX_ASSERT(pThread);
    HX_RELEASE(m_pResponseThread);
    m_pResponseThread = pThread;
}


HXThreadMessageSink* HXTaskManager::Task::GetMessageSink()
{
    return m_pMsgSink;
}

void HXTaskManager::Task::SetMessageSink(HXThreadMessageSink* pMsgSink)
{
    HX_ASSERT(pMsgSink);
    HX_ASSERT(!m_pMsgSink);
    HXRC_RELEASE(m_pMsgSink);
    if( pMsgSink )
    {
        m_pMsgSink = pMsgSink;
        m_pMsgSink->AddRef();
    }
}




HXTaskManager::HXTaskManager(UINT32 taskDoneMsg)
: m_pWorkEvent(0)
, m_pTaskMutex(NULL)
, m_exit(false)
, m_taskDoneMsg(taskDoneMsg) /*unique thread message required per instance*/
, m_pContext(NULL)
{
}


HXTaskManager::~HXTaskManager()
{
    // clean up task thread and sync objects
    DestroyPool();
    HX_RELEASE(m_pWorkEvent);
    if (m_pTaskMutex)
    {
        // clean up outstanding tasks
        HXScopeLock lock(m_pTaskMutex);
        CHXSimpleList::Iterator end = m_pending.End();
        for(CHXSimpleList::Iterator iter = m_pending.Begin(); iter != end; ++iter)
        {
            Task* pTask = reinterpret_cast<Task*>(*iter);
            HXLOGL3(HXLOG_THRD, "HXTaskManager[%p]::~HXTaskManager(): aborting task [%p]", this, pTask);
            pTask->OnTaskComplete(HXR_ABORT); 
            pTask->Release();
        }
        m_pending.RemoveAll();
    }
    HX_ASSERT(m_pending.IsEmpty());

    // un-register with sinks
    CHXSet::Iterator end = m_sinks.End();
    for(CHXSet::Iterator iter = m_sinks.Begin(); iter != end; ++iter)
    {
        HXThreadMessageSink* pSink = reinterpret_cast<HXThreadMessageSink*>(*iter);
        HX_ASSERT(pSink);
        HX_ASSERT(pSink->IsHandled(m_taskDoneMsg));
        pSink->RemoveHandler(m_taskDoneMsg);
        HXRC_RELEASE(pSink);
    }
    m_sinks.RemoveAll();

    HX_RELEASE(m_pTaskMutex);
    HX_RELEASE(m_pContext);
}

HX_RESULT HXTaskManager::Init(IUnknown* pContext, UINT32 threadCount, UINT32 threadPriority)
{
    HX_ASSERT(threadCount > 0);
 
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    // event used to signal task thread to wake up and start working 
    HX_RESULT hr = CreateEventCCF((void**)&m_pWorkEvent, m_pContext, NULL, TRUE);
    if(FAILED(hr))
    {
        goto exit;
    }

    // mutex for accessing task queue
    hr = CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pTaskMutex, m_pContext);
    if(FAILED(hr))
    {
        goto exit;
    }

    // create worker threads
    hr = CreatePool(threadCount, threadPriority);
    if(FAILED(hr))
    {
        goto exit;
    }

exit:
    
    return hr;
}

HX_RESULT HXTaskManager::RemoveTask(Task* pTask)
{
    HX_ASSERT(pTask);
    HXScopeLock lock(m_pTaskMutex);

    LISTPOSITION pos = m_pending.Find(pTask);
    if(pos)
    {
        HXLOGL3(HXLOG_THRD, "HXTaskManager[%p]::RemoveTask(): removing task [%p]", this, pTask);
        m_pending.RemoveAt(pos);
        pTask->OnTaskComplete(HXR_ABORT);
        pTask->Release();
        return HXR_OK;
    }

    HXLOGL3(HXLOG_THRD, "HXTaskManager[%p]::RemoveTask(): no task [%p] (already started or complete)", this, pTask);
    return HXR_FAIL;
}



HX_RESULT HXTaskManager::AddTask(Task* pTask)
{
    HX_ASSERT(m_threads.GetCount() > 0);
    HX_ASSERT(pTask);

    HXThreadMessageSink* pMsgSink = NULL;
    HX_RESULT hr = HXThreadMessageSink::GetThreadInstance(pMsgSink, m_pContext);
    if (FAILED(hr))
    {
        HX_ASSERT(false);
        return hr;
    }

    if( !pMsgSink->IsHandled(m_taskDoneMsg) )
    {
        // register with message sink in order to handle
        // task done messages sent to response thread
        hr = pMsgSink->AddHandler(m_taskDoneMsg, this);
        HX_ASSERT(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
        {
            // keep track of sinks that we have registered with
            HX_ASSERT(!m_sinks.Lookup(pMsgSink));
            pMsgSink->AddRef();
            m_sinks.Add(pMsgSink);
        }
    }

    HX_ASSERT(m_sinks.Lookup(pMsgSink));

    if (SUCCEEDED(hr))
    {
        pTask->AddRef();
        pTask->SetMessageSink(pMsgSink);
        if (!pTask->GetResponseThread())
        {
            IHXThread* pResponseThread = NULL;
	    hr = CreateInstanceCCF(CLSID_IHXThread, (void**)&pResponseThread, m_pContext);
            HX_ASSERT(SUCCEEDED(hr));
            if (SUCCEEDED(hr) )
            {
                // transfer ownership of thread
                pTask->SetResponseThread(pResponseThread);
            }
        }
        

        // add to pending task list and tell a task thread to start working
        HXScopeLock lock(m_pTaskMutex);
        HXLOGL3(HXLOG_THRD, "HXTaskManager[%p]::AddTask(): adding task #%lu", this, m_pending.GetCount());
        m_pending.AddTail(pTask);
        
        // XXXLCM use a semaphore instead; more appropriate
        if (m_pending.GetCount() == 1)
        {
            m_pWorkEvent->SignalEvent();
        }
    }
    HXRC_RELEASE(pMsgSink);
    
    return hr;
}

UINT32 HXTaskManager::GetPendingTaskCount() const
{
    HXScopeLock lock(m_pTaskMutex);
    return m_pending.GetCount();

}


HX_RESULT HXTaskManager::CreateTaskThread(IHXThread*& pThread, UINT32 threadPriority)
{
    HXLOGL3(HXLOG_THRD, "HXTaskManager[%p]::CreateTaskThread(): creating thread (priority %lu)...", this, threadPriority);

    HX_ASSERT(!pThread);
    HX_RESULT hr = CreateInstanceCCF(CLSID_IHXThread, (void**)&pThread, m_pContext);
    if(HXR_OK == hr)
    {
        hr = pThread->CreateThread(TaskThreadProc_, this, 0);
        if(SUCCEEDED(hr))
        {
            pThread->SetThreadName("HXTaskManager Thread");
            hr = pThread->SetPriority(threadPriority);
        }

        if (FAILED(hr))
        {
            HX_ASSERT(false);
            HX_RELEASE(pThread);
        }
    }
    return hr;
}

HX_RESULT HXTaskManager::CreatePool(UINT32 count, UINT32 threadPriority)
{
    HXLOGL3(HXLOG_THRD, "HXTaskManager[%p]::CreatePool(): creating %lu threads...", this, count);
    HX_RESULT hr = HXR_FAIL;
    for(UINT32 idx = 0; idx < count; ++idx)
    {
        IHXThread* pThread = 0;
        hr = CreateTaskThread(pThread, threadPriority);
        if(FAILED(hr))
        {
            HX_ASSERT(false);
            DestroyPool();
            break;
        }
        m_threads.Add(pThread);
    }
    return hr;
}

void HXTaskManager::DestroyPool()
{
    if(!m_threads.IsEmpty())
    {
        // signal task threads to exit
        m_exit = true;
        m_pWorkEvent->SignalEvent();

        CHXSet::Iterator end = m_threads.End();
        for(CHXSet::Iterator begin = m_threads.Begin(); begin != end; ++begin)
        {
            // wait for thread to exit gracefully 
            IHXThread* pThread = reinterpret_cast<IHXThread*>(*begin);
	    pThread->Exit(0); //XXXLCM we probably should kill after timeout
            HXLOGL3(HXLOG_THRD, "HXTaskManager[%p]::DestroyPool(): thread [%p] exited", this, pThread);
	    HX_RELEASE(pThread);
     
        }
        m_threads.RemoveAll();
        HXLOGL3(HXLOG_THRD, "HXTaskManager[%p]::DestroyPool(): %lu tasks abandoned", this, GetPendingTaskCount());
    }
}

//
// called on task thread
//
HXTaskManager::Task* HXTaskManager::GetNextTask()
{
    Task* pTask = 0;
    HXScopeLock lock(m_pTaskMutex);
    if(!m_pending.IsEmpty())
    {
        pTask = reinterpret_cast<Task*>(m_pending.RemoveHead());
        if(m_pending.IsEmpty())
        {
            // no more tasks
            HXLOGL3(HXLOG_THRD, "HXTaskManager[%p]::GetNextTask(): no more tasks...", this);
            m_pWorkEvent->ResetEvent();
        }
    }
    
    return pTask;
}

void* HXTaskManager::TaskThreadProc_(void* pv)
{
    HXTaskManager* pThis = reinterpret_cast<HXTaskManager*>(pv);
    HX_ASSERT(pThis);
    return pThis->TaskThreadProc();  
}
void* HXTaskManager::TaskThreadProc()
{
    for ( ; ; )
    {
        HXLOGL3(HXLOG_THRD, "HXTaskManager[%p]::TaskThreadProc(): waiting for work event...", this);

        // sleep until there is work to do
        m_pWorkEvent->Wait(ALLFS /*INFINITE*/);
        
        HXLOGL3(HXLOG_THRD, "HXTaskManager[%p]::TaskThreadProc(): got work event...", this);

        if (m_exit)
        {
            HXLOGL3(HXLOG_THRD, "HXTaskManager[%p]::TaskThreadProc(): exiting...", this);
            break;
        }
        
        // fetch and execute next task
        Task* pTask = GetNextTask();
        if (pTask)
        {
            pTask->Execute();

            IHXThread* pResponseThread = pTask->GetResponseThread();
            HXThreadMessageSink* pMsgSink = pTask->GetMessageSink();

            HX_ASSERT(pResponseThread && pMsgSink);
            if (pResponseThread && pMsgSink)
            {
                // post response to (response) thread
                HXThreadMessage msgResponse(m_taskDoneMsg, pTask, 0);
                pResponseThread->PostMessage(&msgResponse, pMsgSink->GetSinkHandle());
            }
            
        }        
    }
    
    return 0;
}



//
// HXThreadMessageSink::MessageHandler
//
// Called on parent thread after the resolver thread posts m_taskDoneMsg
//
UINT32 HXTaskManager::HandleMessage(const HXThreadMessage& msg)
{
    HX_ASSERT(m_taskDoneMsg == msg.m_ulMessage);

    // get message data
    Task* pTask = reinterpret_cast<Task*>(msg.m_pParam1);
    HX_ASSERT(pTask);
       
    // notify completion of task
    pTask->OnTaskComplete(HXR_OK);
    pTask->Release();

    return 0;
}



