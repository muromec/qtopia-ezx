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
#include "hlxclib/windows.h"

#include "debug.h"
#include "hxassert.h"
#include "hxthread.h"
#include "hxscope_lock.h"
#include "hxheap.h"
#include "hxstrutl.h"
#include "hxtlogutil.h"
#include "hxscope_lock.h"
#include "hlxosstr.h"

#include "hxwinthreadmessagesink.h"

// offset to window data for this pointer
const UINT32 HXTMS_WINLONG_THISPTR = 0;

#if !defined(SetWindowLongPtr)
// missing on VC6 with no platform sdk installed
#define SetWindowLongPtr SetWindowLong
#define GetWindowLongPtr GetWindowLong
typedef LONG LONG_PTR;
#endif


#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif	

HX_RESULT HXThreadMessageSink::Create(HXThreadMessageSink*& pMsgSink, IUnknown* pContext)
{
    HX_RESULT hr = HXR_FAIL;

    pMsgSink = new HXWinThreadMessageSink();
    if( pMsgSink )
    {
        pMsgSink->AddRef();
        hr = pMsgSink->Init(pContext);
        if(HXR_OK != hr)
        {
            HX_ASSERT(false);
            HX_RELEASE(pMsgSink);
        }
    }
    else 
    {
        hr = HXR_OUTOFMEMORY;
    }
    return hr;
}


HXWinThreadMessageSink::HXWinThreadMessageSink()
{
}

HXWinThreadMessageSink::~HXWinThreadMessageSink()
{
    DWORD   dwRet = 0;
    LRESULT res = 0;

    if (m_handle)
    {
        HXLOGL3(HXLOG_THRD, "HXWinThreadMessageSink[%p]::~HXWinThreadMessageSink() destroying window [%08x]...", this, m_handle);
        // remove this ptr association with window
        SetWindowLongPtr(m_handle, HXTMS_WINLONG_THISPTR, 0);

        // tell window to close (destruct)
        // 
        // don't use PostMessage, PostMessage() returns immediately without waiting 
        // for WM_CLOSE processed, this causes crash in situation where the owner of 
        // this object is a DLL, the DLL is unloaded but the WM_CLOSE message is still
        // on the message queue, because the Window created by HXWinThreadMessageSink 
        // is not closed, the system then tries to call HXWinThreadMessageSink::WindowProc 
        // which is no longer valid after the DLL is unloaded.
        res = SendMessageTimeout(m_handle, WM_CLOSE, 0, 0, SMTO_NORMAL, 1000, &dwRet);
        // investigate the following assert, it means the WM_CLOSE message sent above
        // was not processed successfully within reasonable amount of time(1000ms)
        //
        // XXXMEH - Thread message sinks go away when the media platform is destroyed.
        // However, the thread that this message sink was created for may be long
        // gone by that time and the WM_CLOSE message won't get pumped. There is currently
        // no way to tell net services to destroy the thread message sink for
        // a given thread. Therefore, I believe this assert it innocuous for those
        // kinds of threads.
//        HX_ASSERT(0 != res);
    }
}

HX_RESULT HXWinThreadMessageSink::Init(IUnknown* pContext)
{
    HX_RESULT hr = HXThreadMessageSink::Init(pContext);
    if (FAILED(hr))
    {
        return hr;
    }
    
    return CreateActualWindow();
}

HX_RESULT HXWinThreadMessageSink::CreateActualWindow()
{
    HX_ASSERT(!m_handle);

    HX_RESULT hr = HXR_FAIL;

    CHXString strClassName = "HelixMessageWindowClass";
    strClassName.AppendULONG((ULONG32) &WindowProc);

    DWORD tid = ::GetCurrentThreadId();
    CHXString windowName = "HelixMessageWindow[";
    windowName.AppendULONG(tid);
    windowName += "]";

    HMODULE hModule = ::GetModuleHandle(NULL);
    
    WNDCLASS wndClass;
    memset(&wndClass, 0, sizeof(wndClass));

                              
    wndClass.lpfnWndProc 	= WindowProc;
    wndClass.cbWndExtra	        = sizeof(this);
    wndClass.hInstance	        = hModule;
    wndClass.lpszClassName	= OS_STRING(strClassName);
    
    // ensure class is registered
    ATOM atom = ::RegisterClass(&wndClass);
    if (!atom)
    {
        DWORD err = ::GetLastError();
        HX_ASSERT(ERROR_CLASS_ALREADY_EXISTS == err);
    }

    //	create an instance of the window	
    HX_ASSERT(!m_handle);
    m_handle = ::CreateWindow(OS_STRING(strClassName), OS_STRING(windowName), 
		WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, hModule, NULL);
 
    HX_ASSERT(m_handle);
    if (m_handle)
    {
        // associate this ptr with window
        SetWindowLongPtr(m_handle, HXTMS_WINLONG_THISPTR, reinterpret_cast<LONG_PTR>(this));
        HXLOGL3(HXLOG_THRD, "HXWinThreadMessageSink[%p]::CreateActualWindow():  '%s':  hwnd [%08x] for instance [%p]", this, static_cast<const char*>(windowName), m_handle, GetWindowLongPtr(m_handle, HXTMS_WINLONG_THISPTR));
	hr = HXR_OK;
    }


    return hr;
}


LRESULT HXWinThreadMessageSink::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    {
    HXScopeLock lock(m_pMutex);
    void* pv = 0;
    if (m_handlers.Lookup(reinterpret_cast<void*>(msg), pv))
    {
        MessageHandler* pHandler = reinterpret_cast<MessageHandler*>(pv);
        HX_ASSERT(pHandler);
        if  (pHandler)
        {
            HXThreadMessage hxtm(msg, reinterpret_cast<void*>(wParam), reinterpret_cast<void*>(lParam));
            return static_cast<LRESULT>(pHandler->HandleMessage(hxtm));
        }
    }
    }
    
    return ::DefWindowProc(m_handle, msg, wParam, lParam);

}


LRESULT CALLBACK HXWinThreadMessageSink::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // retrieve 'this' pointer
    HXWinThreadMessageSink* pThis = reinterpret_cast<HXWinThreadMessageSink*>(GetWindowLongPtr(hWnd, HXTMS_WINLONG_THISPTR));
    if (pThis)
    {
        // ensure that 'this' remains valid for duration of call to HandleMessage()
        pThis->AddRef();
        LRESULT res = pThis->HandleMessage(msg, wParam, lParam);
        HX_RELEASE(pThis);
        return res;
    }
    
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
