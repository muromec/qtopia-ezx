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

/*
 *
 * Synopsis:
 *
 * HXTaskManager executes blocking or long running tasks (functor-like object
 * derived from HXTaskManager::Task) on a task thread. The task thread is one
 * of a pool of pre-created threads. When a task is complete the owner thread
 * is notified asynchronously via a message posted to the owner thread.
 *
 */

#if !defined( HX_TASKMANAGER_H__ )
#define	HX_TASKMANAGER_H__

#include "hxslist.h"
#include "hxthreadmessagesink.h"
#include "hxset.h"
#include "hxrefcounted.h"

class HXThread;
class HXEvent;
class HXThreadMessageSink;
class HXMutex;


class HXTaskManager
: public HXRefCounted
, public HXThreadMessageSink::MessageHandler
{
public:
    class Task : public HXRefCounted
    {
    public:
        Task();
        
        
        // Called on task thread
        virtual void Execute() = 0;

        //
        // Called on parent thread. If the task completed
        // execution, 'hr' is HXR_OK; otherwise HXR_ABORT.
        // It does not indicate the result of the operation
        // itself which may be defined by the derived task.
        //
        virtual void OnTaskComplete(HX_RESULT hr) = 0;


        IHXThread* GetResponseThread();
        void	   SetResponseThread(IHXThread* pThread);
        
        HXThreadMessageSink* GetMessageSink();
        void                 SetMessageSink(HXThreadMessageSink* pMsgSink);

      protected:
        virtual ~Task();
        IHXThread*           m_pResponseThread;
        HXThreadMessageSink* m_pMsgSink;
        
    };

public:
    HXTaskManager(UINT32 taskDoneMsg);
    
// HXTaskManager
    HX_RESULT Init(IUnknown* pContext, UINT32 threadCount, UINT32 threadPriority = 0 /*THREAD_PRIORITY_ABOVE_NORMAL*/); //XXXLCM need HX_THREAD_PRIORITY
    HX_RESULT AddTask(Task* pTask);
    HX_RESULT RemoveTask(Task* pTask);
    UINT32 GetPendingTaskCount() const;

protected:
    virtual ~HXTaskManager();

private:
// HXThreadMessageSink::MessageHandler
    UINT32 HandleMessage(const HXThreadMessage& msg);

// implementation
    Task* GetNextTask();
    HX_RESULT CreateTaskThread(IHXThread*& pThread, UINT32 threadPriority);
    HX_RESULT CreatePool(UINT32 threadCount, UINT32 threadPriority);
    void DestroyPool();

// task thread
    static void* TaskThreadProc_(void* pv);
    void* TaskThreadProc();
    friend void* TaskThreadProc_(void* pv);

private:

    // pool of worker threads that run tasks
    CHXSet              m_threads;          
    IHXEvent*           m_pWorkEvent;
    IUnknown*		m_pContext;
    IHXMutex*           m_pTaskMutex;

    // collection of pending tasks 
    CHXSimpleList       m_pending;          
    bool                m_exit;

    // unique message used by msg sinks to notify this instance on parent thread
    UINT32              m_taskDoneMsg;
    
    // sinks that we have registered with
    CHXSet              m_sinks; 
};
   

#endif	// HX_TASKMANAGER_H__
