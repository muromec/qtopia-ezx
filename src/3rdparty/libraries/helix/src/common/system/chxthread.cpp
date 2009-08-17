/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxthread.cpp,v 1.7 2006/12/06 10:33:12 gahluwalia Exp $
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

#include "chxthread.h"
#include "hxthread.h"
#include "pckunpck.h"

CHXThread::CHXThread()
	  :m_lRefCount(0)
	  ,m_pThread(NULL)
{
#if defined(THREADS_SUPPORTED) || defined(_UNIX_THREADS_SUPPORTED)
    HXThread::MakeThread(m_pThread);
#else
    HXThread::MakeStubThread(m_pThread);
#endif
}

CHXThread::~CHXThread()
{
    HX_DELETE(m_pThread);
}

STDMETHODIMP CHXThread::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXThread), (IHXThread*)this },
            { GET_IIDHANDLE(IID_IHXNativeThread), (IHXNativeThread*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXThread*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) CHXThread::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXThread::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
CHXThread::CreateThread(void* (pExecAddr(void*)), 
			void* pArg,
			ULONG32 ulCreationFlags)
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	rc = m_pThread->CreateThread(pExecAddr, pArg, ulCreationFlags);
    }
    return rc;
}

STDMETHODIMP
CHXThread::Suspend()
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	rc = m_pThread->Suspend();
    }
    return rc;
}

STDMETHODIMP
CHXThread::Resume()
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	rc = m_pThread->Resume();
    }
    return rc;
}

// the following methods are mimic pthread_cancel & pthread_join 
STDMETHODIMP
CHXThread::Cancel()
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	rc = m_pThread->Cancel();
    }
    return rc;
}

STDMETHODIMP
CHXThread::Terminate()
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	rc = m_pThread->Terminate();
    }
    return rc;
}

STDMETHODIMP
CHXThread::SetPriority(UINT32 ulPriority)
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	rc = m_pThread->SetPriority(ulPriority);
    }
    return rc;
}

STDMETHODIMP
CHXThread::GetPriority(UINT32& ulPriority)
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	rc = m_pThread->GetPriority(ulPriority);
    }
    return rc;
}
    
STDMETHODIMP
CHXThread::YieldTimeSlice()
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	rc = m_pThread->YieldTimeSlice();
    }
    return rc;
}

STDMETHODIMP
CHXThread::Exit(UINT32 ulExitCode)
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	rc = m_pThread->Exit(ulExitCode);
    }
    return rc;
}

STDMETHODIMP
CHXThread::GetThreadId(UINT32& ulThreadId)
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	rc = m_pThread->GetThreadId(ulThreadId);
    }
    return rc;
}

STDMETHODIMP
CHXThread::SetThreadName(const char* pszName)
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	m_pThread->SetThreadName(pszName);
	rc = HXR_OK;
    }
    return rc;
}
    
STDMETHODIMP
CHXThread::GetThreadName(const char*& pszName)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXThread::PostMessage(HXThreadMessage* pMsg, 
		       void* pWindowHandle)
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	rc = m_pThread->PostMessage(pMsg, pWindowHandle);
    }
    return rc;
}

STDMETHODIMP
CHXThread::GetMessage(HXThreadMessage* pMsg, 
		      UINT32 ulMsgFilterMin, 
		      UINT32 ulMsgFilterMax)
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	rc = m_pThread->GetMessage(pMsg, ulMsgFilterMin, ulMsgFilterMax);
    }
    return rc;
}

STDMETHODIMP
CHXThread::PeekMessage(HXThreadMessage* pMsg, 
		       UINT32 ulMsgFilterMin, 
		       UINT32 ulMsgFilterMax,
		       HXBOOL bRemoveMessage)
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	rc = m_pThread->PeekMessage(pMsg, ulMsgFilterMin, ulMsgFilterMax, bRemoveMessage);
    }
    return rc;
}

STDMETHODIMP
CHXThread::PeekMessageMatching(HXThreadMessage* pMsg,
                               HXThreadMessage* pMatch,
			       HXBOOL bRemoveMessage)
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	rc = m_pThread->PeekMessageMatching(pMsg, pMatch, bRemoveMessage);
    }
    return rc;
}

STDMETHODIMP
CHXThread::DispatchMessage(HXThreadMessage* pMsg)
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pThread)
    {
	rc = m_pThread->DispatchMessage(pMsg);
    }
    return rc;
}

CHelixEvent::CHelixEvent()
	    :m_lRefCount(0)
	    ,m_pEvent(NULL)
{
}

CHelixEvent::~CHelixEvent()
{
    HX_DELETE(m_pEvent);
}

STDMETHODIMP 
CHelixEvent::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXEvent), (IHXEvent*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXEvent*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

ULONG32 CHelixEvent::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG32 CHelixEvent::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *	IHXEvent methods
 */
STDMETHODIMP
CHelixEvent::Init(const char*	pszName,
		  HXBOOL	bManualReset)
{
    HX_RESULT	rc = HXR_OK;

    if (m_pEvent)
    {
	rc = HXR_ALREADY_INITIALIZED;
	goto exit;
    }

#if defined(THREADS_SUPPORTED) || defined(_UNIX_THREADS_SUPPORTED)
    HXEvent::MakeEvent(m_pEvent, pszName, bManualReset);
#else
    HXEvent::MakeStubEvent(m_pEvent, pszName, bManualReset);
#endif
   
    if (!m_pEvent)
    {
	rc = HXR_FAILED;
    }

exit:
    return rc;
}

STDMETHODIMP
CHelixEvent::SignalEvent()
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pEvent)
    {
	rc = m_pEvent->SignalEvent();
    }
    return rc;
}

STDMETHODIMP
CHelixEvent::ResetEvent()
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pEvent)
    {
	rc = m_pEvent->ResetEvent();
    }
    return rc;
}

STDMETHODIMP_(void*)
CHelixEvent::GetEventHandle()
{
    void* pHandle = NULL;

    if (m_pEvent)
    {
	pHandle = m_pEvent->GetEventHandle();
    }
    return pHandle;
}

STDMETHODIMP
CHelixEvent::Wait(UINT32 ulTimeoutPeriod)
{
    HX_RESULT rc = HXR_FAIL;

    if (m_pEvent)
    {
	rc = m_pEvent->Wait(ulTimeoutPeriod);
    }
    return rc;
}


//
// CHXAsyncTimer
//
CHXAsyncTimer::CHXAsyncTimer(IUnknown* pContext)
    : m_lRefCount(0)
    , m_pContext(pContext)
{
    HX_ADDREF(m_pContext);
}


CHXAsyncTimer::~CHXAsyncTimer()
{
    //XXXgfw if a timer is left running what should we do?  It depends
    //on how we think these CHXAsyncTimer objects are to act. If they
    //are to mimic the static behaviour of HXAsyncTimers then
    //nothing. If we think CHXAsyncTimers wrap one and only one
    //HXAsyncTimer call, then we should probably save the timer id and
    //kill it here. If we did that then that would require users to
    //create a new CHXAsyncTimer object for each timer they wanted to
    //set. Right now, we let the user create one and do as many set
    //and kill timers as needed.
    HX_RELEASE(m_pContext);
}

STDMETHODIMP CHXAsyncTimer::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXAsyncTimer), (IHXAsyncTimer*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXAsyncTimer*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) CHXAsyncTimer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXAsyncTimer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP_(UINT32)
CHXAsyncTimer::SetTimer(UINT32 ulTimeOut, IHXThread* pReceivingThread)
{
    IHXNativeThread* pNT  = NULL;
    UINT32           ulID = 0;
        
        
    HX_ASSERT( ulTimeOut );
    HX_ASSERT( pReceivingThread );
    
    pReceivingThread->QueryInterface(IID_IHXNativeThread, (void**)&pNT);
    if( pNT )
    {
        HXThread* pHXThread = pNT->GetHXThread();
        HX_RELEASE(pNT);
        ulID = HXAsyncTimer::SetTimer(ulTimeOut, pHXThread, m_pContext);
    }
    return ulID;
}

STDMETHODIMP_(UINT32)
CHXAsyncTimer::SetTimer(UINT32 ulTimeOut, TIMERPROC pfExecFunc)
{
    UINT32 ulID = 0;
        
    HX_ASSERT( ulTimeOut );
    HX_ASSERT( pfExecFunc );
    
    ulID = HXAsyncTimer::SetTimer(ulTimeOut, pfExecFunc,  m_pContext);
    return ulID;
    
}

STDMETHODIMP CHXAsyncTimer::KillTimer(ULONG32 ulID)
{
    HXAsyncTimer::KillTimer(ulID);
    return HXR_OK;
}

