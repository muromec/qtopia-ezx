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
#include "hxmsgs.h"
#include "hxtick.h"
#include "pckunpck.h"
#include "hxthread.h"
#include "hxthreadtask.h"
#include "hxthreadtaskdriver.h"

#include "hxtlogutil.h"
#include "hxassert.h"
#include "debug.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


// XXXLCM need cross-platfrom priority values (e.g., in HXThread headers)
#if defined(WINDOWS)
#define HX_THREAD_PRIORITY_NORMAL           THREAD_PRIORITY_NORMAL
#define HX_THREAD_PRIORITY_HIGHEST          THREAD_PRIORITY_HIGHEST
#define HX_THREAD_PRIORITY_ABOVE_NORMAL     THREAD_PRIORITY_ABOVE_NORMAL
#else
#define HX_THREAD_PRIORITY_NORMAL           0
#define HX_THREAD_PRIORITY_HIGHEST          0
#define HX_THREAD_PRIORITY_ABOVE_NORMAL     0
#endif

static HXThreadTaskDriver* g_pDriver = NULL;

HX_RESULT HXThreadTaskDriver::GetInstance(HXThreadTaskDriver*& pDriver, IUnknown* pContext)
{
    HX_RESULT hr = HXR_OK;
    if( g_pDriver )
    {
        g_pDriver->AddRef();
        pDriver = g_pDriver;
    }
    else
    {
        // HXMSG_THREADEDSOCK is message sent back and forth between parent and driver thread
        g_pDriver = new HXThreadTaskDriver(HXMSG_THREADEDSOCK);
        if(g_pDriver)
        {
            g_pDriver->AddRef();
            hr = g_pDriver->Init(HX_THREAD_PRIORITY_NORMAL, pContext);
            if(FAILED(hr))
            {
                HXRC_RELEASE(g_pDriver);
            }
        }
        else
        {
            hr = HXR_OUTOFMEMORY;
        }
        pDriver = g_pDriver;
    }

    return hr;
}


void HXThreadTaskDriver::FinalRelease()
{
    g_pDriver = 0;
    HXRefCounted::FinalRelease();
}



HXThreadTaskDriver::HXThreadTaskDriver(UINT32 driverTaskMsg)
: m_pContext(NULL)
, m_pParentThread(0)
, m_pDriverThread(0)
, m_pMsgSink(0)
, m_driverTaskMsg(driverTaskMsg)
{
}


HXThreadTaskDriver::~HXThreadTaskDriver()
{
    if(m_pDriverThread)
    {
        // wait for thread to exit gracefully
        HXThreadMessage msg(HXMSG_QUIT, 0, 0);
        m_pDriverThread->PostMessage(&msg, NULL);
        m_pDriverThread->Exit(0);
        HX_RELEASE(m_pDriverThread);
    }
    HX_RELEASE(m_pParentThread);
    if(m_pMsgSink)
    {
        m_pMsgSink->RemoveHandler(m_driverTaskMsg);
        HXRC_RELEASE(m_pMsgSink);
    }
    HX_RELEASE(m_pContext);
}

HX_RESULT HXThreadTaskDriver::SetThreadPriority(UINT32 threadPriority)
{
    HX_ASSERT(m_pDriverThread); // call Init() first
    HXLOGL3(HXLOG_THRD, "HXThreadTaskDriver[%p]::SetThreadPriority(): priority = %lu", this, threadPriority);
    return m_pDriverThread->SetPriority(threadPriority);
}



HX_RESULT HXThreadTaskDriver::Init(UINT32 threadPriority, IUnknown* pContext)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    // creat parent thread wrapper
    HX_RESULT hr = CreateInstanceCCF(CLSID_IHXThread, (void**)&m_pParentThread, pContext);
    if(FAILED(hr))
    {
        goto exit;
    }

    // creat driver thread
    HX_ASSERT(!m_pDriverThread);
    hr = CreateInstanceCCF(CLSID_IHXThread, (void**)&m_pDriverThread, pContext);
    if(FAILED(hr))
    {
        goto exit;
    }

    hr = m_pDriverThread->CreateThread(ThreadProc_, this, 0);
    if(FAILED(hr))
    {
        goto exit;
    }

    m_pDriverThread->SetThreadName("HXThreadTaskDriver Thread");

    hr = SetThreadPriority(threadPriority);
    if (FAILED(hr))
    {
        goto exit;
    }

        
    //
    // Register to receive messages sent from driver thread
    // to the message sink associated with this thread.
    //
    hr = HXThreadMessageSink::GetThreadInstance(m_pMsgSink, pContext);
    if(SUCCEEDED(hr))
    {
        hr = m_pMsgSink->AddHandler(m_driverTaskMsg, this);
    }

exit:
    if(FAILED(hr))
    {
        HX_ASSERT(false);
        HX_RELEASE(m_pDriverThread);
    }

    return hr;
}


HX_RESULT HXThreadTaskDriver::SendAndWait(HXThreadTask* pTask)
{
    HX_ASSERT(pTask);

    pTask->AddRef(); // transfer ownership to driver thread
    HX_RESULT hr = pTask->EnableWait(m_pContext);
    if (SUCCEEDED(hr))
    {
        HXThreadMessage hxtm(HXMSG_THREADEDSOCK, pTask, 0);

        // block and wait for task to complete
        hr = m_pDriverThread->PostMessage(&hxtm, NULL);
        HX_ASSERT(HXR_OK == hr);
        if(SUCCEEDED(hr))
        {
#if defined(HELIX_FEATURE_LOGLEVEL_4) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
            UINT32 start = HX_GET_TICKCOUNT();
#endif
            hr = pTask->Wait();
            HX_ASSERT(HXR_OK == hr);
            if(SUCCEEDED(hr))
            {

                // normally this should be HXR_OK indicating task was executed
                hr = pTask->GetResult();
                HX_ASSERT(HXR_OK == hr);
            }
#if defined(HELIX_FEATURE_LOGLEVEL_4) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
            HXLOGL4(HXLOG_THRD, "HXThreadTaskDriver[%p]::SendAndWait(): task [%p]: waited %ul ms", this, pTask, CALCULATE_ELAPSED_TICKS(start, HX_GET_TICKCOUNT()));
#endif

        }
        else
        {
            // since we never transferred ownership to driver thread
            pTask->Release();
        }
    }

    
    return hr;
}


HX_RESULT HXThreadTaskDriver::Send(HXThreadTask* pTask)
{
    HX_ASSERT(pTask);

    pTask->AddRef(); // transfer ownership to driver thread

    HXThreadMessage hxtm(m_driverTaskMsg, pTask, 0);
    return m_pDriverThread->PostMessage(&hxtm, NULL);
}

//
// HXThreadMessageSink::MessageHandler
//
// Called by message sink for this thread when driver thread sends a message
//
UINT32 HXThreadTaskDriver::HandleMessage(const HXThreadMessage& msg)
{
    HX_ASSERT(m_driverTaskMsg == msg.m_ulMessage);
    if(m_driverTaskMsg == msg.m_ulMessage)
    {
        // execute task on this thread
        HXThreadTask* pTask = reinterpret_cast<HXThreadTask*>(msg.m_pParam1);
        HX_ASSERT(pTask);
        pTask->Execute();
        pTask->Complete();
        pTask->Release();
    }
  
    return 0;
}

// [driver thread]
HX_RESULT HXThreadTaskDriver::SendParent(HXThreadTask* pTask)
{
    HXThreadMessage hxtm(m_driverTaskMsg, pTask, 0);
    
    pTask->AddRef(); // transfer ownership to parent thread

    HX_RESULT hr = m_pParentThread->PostMessage(&hxtm, m_pMsgSink->GetSinkHandle());
    if( FAILED(hr))
    {
        pTask->Release();
    }
    return hr;

}

// [driver thread]
void* HXThreadTaskDriver::ThreadProc_(void* pv)
{
    HXThreadTaskDriver* pThis = reinterpret_cast<HXThreadTaskDriver*>(pv);
    HX_ASSERT(pThis);
    return pThis->ThreadProc();  
}
void* HXThreadTaskDriver::ThreadProc()
{
    HXLOGL3(HXLOG_THRD, "HXThreadTaskDriver[%p]::ThreadProc(): entering message loop...", this);

    //
    // Execute each task that is sent to this
    // thread and dispatch everything else.
    //
    HXThreadMessage msg;
    while (m_pDriverThread->GetMessage(&msg, 0, 0) == HXR_OK)
    {
	if(msg.m_ulMessage == m_driverTaskMsg)
        {
            // driver task
            HXThreadTask* pTask = reinterpret_cast<HXThreadTask*>(msg.m_pParam1);
            HX_ASSERT(pTask);

            pTask->Execute();
            pTask->Complete();

            pTask->Release();
        }
        else if(msg.m_ulMessage == HXMSG_QUIT)
        {
            // exit thread
            HXLOGL3(HXLOG_THRD, "HXThreadTaskDriver[%p]::ThreadProc(): got HXMSG_QUIT", this);
            break;
        }
        else
        {
            m_pDriverThread->DispatchMessage(&msg);
	}  
    }

    HXLOGL3(HXLOG_THRD, "HXThreadTaskDriver[%p]::ThreadProc(): exiting...", this);
    return 0;
}





