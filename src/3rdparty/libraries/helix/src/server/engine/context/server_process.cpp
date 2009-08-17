/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_process.cpp,v 1.5 2005/08/05 01:10:16 atin Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
/*
 *  Implementation of Server's IHXProcess object
 *
 */

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxengin.h"
#include "simple_callback.h"
#include "dispatchq.h"
#include "server_process.h"
#include "server_context.h"
#include "server_inetwork.h"
#include "proc_container.h"
#include "microsleep.h"

extern int* VOLATILE pHXProcInitMutex;

#if !defined(_WIN32)
extern int* MakeProcess(const char* processname,
                        SimpleCallback* cb,
                        DispatchQueue* dispatch_queue,
                        int flags = 0,
                        void* linuxstuff = 0);
#else
extern int* MakeProcess(const char* processname,
                        SimpleCallback* cb,
                        DispatchQueue* dispatch_queue,
                        int flags = 0);
#endif

/////////////////////////////////////////
// helper classes used only in this file
/////////////////////////////////////////
class ProcessCallback : public SimpleCallback
{
public:
    ProcessCallback(IHXProcessEntryPoint* pUserEntryPoint, Process* pCopyProc);
    void func(Process* pProcess);

private:
    ~ProcessCallback();

    IHXProcessEntryPoint* m_pUserEntryPoint;
    Process*               m_pCopyProc;
    BOOL m_bInitialized;
};

ProcessCallback::ProcessCallback(IHXProcessEntryPoint* pUserEntryPoint,
                                 Process* pCopyProc)
{
    m_bInitialized = FALSE;
    m_pUserEntryPoint = pUserEntryPoint;
    m_pUserEntryPoint->AddRef();
    m_pCopyProc = pCopyProc;
}

ProcessCallback::~ProcessCallback()
{
    HX_RELEASE(m_pUserEntryPoint);
}

void
ProcessCallback::func(Process* proc)
{
    if (!m_bInitialized)
    {
        // if we're being called from makeprocess, we need to
        // set ourselves up
        m_bInitialized = TRUE;

        proc->pc = new ProcessContainer(proc, m_pCopyProc->pc);
        proc->pc->dispatchq->init(proc);
        proc->pc->process_type = PTWorker;
    
        proc->pc->network_services->Init(proc->pc->server_context,
                                         proc->pc->engine, proc->pc->access_ctrl);

        *pHXProcInitMutex = 0;

        // put a copy of ourselves on our mainloop and then enter the mainloop
        proc->pc->dispatchq->send(proc, this, proc->procnum());
        proc->pc->engine->mainloop();
    }
    else
    {
        // otherwise we're being called from the mainloop. so
        // we stick another copy of ourselves on the queue (for the
        // next time we CA) and then run the user's func.
        proc->pc->dispatchq->send(proc, this, proc->procnum());

        // unlock global mutex as the only time the user function should
        // return is on CA or Exit, otherwise it's an infinite loop
        HXMutexUnlock(g_pServerMainLock);
        proc->pc->engine->m_bMutexProtection = FALSE;

        m_pUserEntryPoint->Func(proc->pc->server_context);

        HXMutexLock(g_pServerMainLock);
        proc->pc->engine->m_bMutexProtection = TRUE;
    }
}

////////////////////////////////////////////////////////////////////////////
// This class is the callback passed to the core proc which runs MakeProcess
////////////////////////////////////////////////////////////////////////////
class CoreProcessCreateCallback : public SimpleCallback
{
public:
    CoreProcessCreateCallback(const char* pProcessName,
                              SimpleCallback* pEntryPoint,
                              Process* pCopyProc);
    void func(Process* pProcess);

    ~CoreProcessCreateCallback();

    SimpleCallback* m_pEntryPoint;
    Process*        m_pCopyProc;
    const char*     m_pProcessName;
};

CoreProcessCreateCallback::CoreProcessCreateCallback(const char* pProcessName,
                                                    SimpleCallback* pEntryPoint,
                                                     Process* pCopyProc)
{
    m_pProcessName = pProcessName;
    m_pCopyProc = pCopyProc;
    m_pEntryPoint = pEntryPoint;
}

CoreProcessCreateCallback::~CoreProcessCreateCallback()
{
}

void
CoreProcessCreateCallback::func(Process* pProcess)
{
    *pHXProcInitMutex = 1;
    MakeProcess(m_pProcessName, m_pEntryPoint, m_pCopyProc->pc->dispatchq);
    while (*pHXProcInitMutex)
    {
        microsleep(1);
    }
}

/////////////////////////
// Main class definition
/////////////////////////

/*
 * ServerHXProcess::ServerHXProcess
 *
 * Description: constructor
 *
 * Implementation: construct!
 */
ServerHXProcess::ServerHXProcess(Process* pCopyProc) 
{
    m_ulRefCount = 0;
    m_pCopyProc = pCopyProc;
}

/*
 * ServerHXProcess::~ServerHXProcess
 *
 * Description: destructor
 *
 * Implementation: destruct!
 */
ServerHXProcess::~ServerHXProcess() 
{
}

/*
 * ServerHXProcess::Start
 *
 * Description:
 * Parameters:
 *      [in]  processName
 *      [in]  entryPoint
 *
 * Returns: HXR_OK on success, failure code otherwise
 *
 * Implementation: send a message to the core proc to call make process
 */
STDMETHODIMP
ServerHXProcess::Start(const char* pProcessName,
                        IHXProcessEntryPoint* pEntryPoint) 
{
    ProcessCallback* pCB = new ProcessCallback(pEntryPoint, m_pCopyProc);
    SimpleCallback *pCoreCB = new CoreProcessCreateCallback(pProcessName, pCB, m_pCopyProc);
    m_pCopyProc->pc->dispatchq->send(m_pCopyProc, pCoreCB, PROC_RM_CONTROLLER);

    return HXR_OK;
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP 
ServerHXProcess::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXProcess))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
ServerHXProcess::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
ServerHXProcess::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
        return m_ulRefCount;

    delete this;
    return 0;
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP 
ThreadLocalInfo::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXThreadLocal))
    {
	AddRef();
	*ppvObj = (IHXThreadLocal*) this;
	return HXR_OK;
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*) this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
ThreadLocalInfo::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
ThreadLocalInfo::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
        return m_ulRefCount;

    delete this;
    return 0;
}

STDMETHODIMP_(int)
ThreadLocalInfo::GetMaxThreads()
{
    return MAX_THREADS;
}

STDMETHODIMP_(int)
ThreadLocalInfo::GetThreadNumber()
{
    return Process::get_procnum();
}

