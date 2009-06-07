/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: casynthr.cpp,v 1.10 2007/04/13 19:50:12 ping Exp $
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

#include "hxtypes.h"
#include "hlxclib/windows.h"

#include "hxresult.h"
#include "hxslist.h"
#include "hxcom.h"
#include "hxengin.h"
#include "hxthread.h"
#include "conn.h"
#include "thrdconn.h"
#include "threngin.h"
#include "platform/win/casynthr.h"	
#include "hxmap.h"	
#include "hxassert.h"	
#include "hxstrutl.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif									 

#ifdef WIN32_PLATFORM_PSPC
#define WM_NCCREATE WM_CREATE
#define WM_NCDESTROY WM_DESTROY
#define	WND_CLASS_BASE	L"HX_Internal_NetThread_Async"
#else
#define	WND_CLASS_BASE	"HX_Internal_NetThread_Async"
#endif

#define	MAX_WND_CLASS_LENGTH	50

#ifdef _WIN32
UINT CAsyncNetThread::zm_uDestroyMessage = 0;
#endif



#define	OFFSET_THIS	0

HXBOOL		    CAsyncNetThread::zm_bClassRegistered    = FALSE;
CAsyncNetThread*    CAsyncNetThread::zm_pSockNotifiers	    = NULL;

CAsyncNetThread*
CAsyncNetThread::GetCAsyncNetThreadNotifier(IUnknown* pContext, HINSTANCE hInst, HXBOOL Create)
{
    CAsyncNetThread* pThis = 0;

    //	Does our current session already has a notifier?
    if (!(pThis = FindNetThreadNotifier()) && Create)
    {
	// No, then create one
	pThis = new CAsyncNetThread(pContext, hInst);
	if (pThis && !pThis->IsValid())
	{
	    //	If creation failed then delete the pointer
	    delete pThis;
	    pThis = NULL;
	}

	if (pThis)
	{
	    LinkUs(pThis);
	}
    }

    return (pThis);
}

ULONG32 CAsyncNetThread::GetSession()
{
#if defined( _WIN32 )
	return (GetCurrentThreadId());
#else
	return ((ULONG32)(LPSTR)GetCurrentTask());
#endif
}

CAsyncNetThread::CAsyncNetThread(IUnknown* pContext, HINSTANCE hInst)
    : m_hWnd(NULL)
    , m_bValid(FALSE)
    , m_hInst(hInst)
    , m_nNumSocketsClients(0)
    , m_bInCancelMode(FALSE)
    , m_pNextNotifier(0)
    , m_pThreadEngineMutex(0)
    , m_pContext(pContext)
{
    HX_ADDREF(m_pContext);

#ifdef _WIN32
    if (!zm_uDestroyMessage)
    {
	zm_uDestroyMessage = RegisterWindowMessage(OS_STRING("HX_DestroyInternalNetThreadAsync"));
    }
#endif

    m_ClientSocketsMap = new CHXMapPtrToPtr;

    m_ulSession = CAsyncNetThread::GetSession();

    m_bValid = Create();
    ThreadEngine* pEngine   = ThreadEngine::GetThreadEngine(m_pContext);
    m_pThreadEngineMutex    = pEngine->GetMutex();
}

//	Workhorse construction method
HXBOOL CAsyncNetThread::Create()
{
	char szClassName[MAX_WND_CLASS_LENGTH]; /* Flawfinder: ignore */

    SafeSprintf(szClassName, MAX_WND_CLASS_LENGTH, OS_STRING("%s%d"), 
	     WND_CLASS_BASE, &zm_bClassRegistered);

	if (!zm_bClassRegistered)
    {
	WNDCLASS internalClass;
    OS_STRING_TYPE osstrClassName(szClassName);

	//	First register our window class                                  
	internalClass.style 		= 0;
	internalClass.lpfnWndProc 	= CAsyncNetThread::AsyncNotifyProc;
	internalClass.cbClsExtra	= 0;
	internalClass.cbWndExtra	= sizeof(this);
	internalClass.hInstance		= m_hInst; // Use app's instance
	internalClass.hIcon		= 0;
	internalClass.hCursor		= 0;
	internalClass.hbrBackground	= 0;
	internalClass.lpszMenuName	= NULL;
	internalClass.lpszClassName	= osstrClassName;
	
	zm_bClassRegistered = RegisterClass(&internalClass);
	
	if(!zm_bClassRegistered)
	{
	    UnregisterClass(OS_STRING(szClassName), m_hInst);
	    zm_bClassRegistered = RegisterClass(&internalClass);
	}
    }

    //	Now create an instance of the window	
    m_hWnd = ::CreateWindow(OS_STRING(szClassName), OS_STRING("RealMedia Internal Messages"), 
		WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, m_hInst, this);
 
    if (!m_hWnd)
    {
	return (FALSE);
    }

    return (TRUE);
}

//	Destructor: Destroys the window and unregisters the class if necessary
CAsyncNetThread::~CAsyncNetThread()
{
    m_bValid = FALSE;

    m_hWnd  = NULL;
    m_hInst = NULL;

    if(m_ClientSocketsMap)
    {
	delete m_ClientSocketsMap;
	m_ClientSocketsMap = NULL;
	m_nNumSocketsClients = 0;
    }

    HX_RELEASE(m_pContext);
}

LRESULT
CAsyncNetThread::AsyncNotifyProc(HWND hWnd, UINT msg, 
				 WPARAM wParam, LPARAM lParam)
{
    CAsyncNetThread*    pThis = 0;

    if (msg == WM_NCCREATE)
    {
	CREATESTRUCT* lpCreate = 0;

	// Set our this pointer, so our WndProc can find us again
	lpCreate = (CREATESTRUCT FAR*) lParam;
	pThis = (CAsyncNetThread*) lpCreate->lpCreateParams;
	SetWindowLong(hWnd, OFFSET_THIS, (long) pThis);
    }
    else if (msg == WM_NCDESTROY)
    {
	// remove our this pointer so if somebody calls this function
	// again after the window is gone (and the object is gone
	// too) we don't try to call a method from the pointer
	SetWindowLong(hWnd, OFFSET_THIS, 0L);
    }
    else
    {
	pThis = (CAsyncNetThread*) (LPHANDLE)GetWindowLong(hWnd, OFFSET_THIS);
    }

    if (pThis != NULL)
    {
	switch (msg)
	{
	    case HXMSG_ASYNC_DNS:
		return (pThis->OnAsyncDNS(wParam, lParam));
	    case HXMSG_ASYNC_CONNECT:
		return (pThis->OnAsyncConnect(wParam, lParam));
	    case HXMSG_ASYNC_READ:
		return (pThis->OnAsyncRead(wParam, lParam));
	    case HXMSG_ASYNC_WRITE:
		return (pThis->OnAsyncWrite(wParam, lParam));
	    case HXMSG_ASYNC_ACCEPT:
		return (pThis->OnAsyncAccept(wParam, lParam));
	    default:
		break;
	}
    }

#ifdef _WIN32
    if (msg == zm_uDestroyMessage)
    {
	LRESULT result = DestroyWindow(hWnd);
	delete pThis;
	return result;
    }
#endif

    return (DefWindowProc(hWnd, msg, wParam, lParam));
}

LRESULT CAsyncNetThread::CheckClients()
{
    if (!m_bInCancelMode && m_nNumSocketsClients == 0)
    {
	MSG msg;
	while (m_hWnd && PeekMessage(&msg, m_hWnd,0,0,PM_REMOVE))
	{
	    if(msg.message == WM_QUIT) 
	    {   
		// When peeking WM_QUIT message in the main thread of an application 
		// we have to put it back into message queue to allow the application 
		// to exit correctly. SB
		PostQuitMessage(0);
		break;
	    }
	    else
	    {
		// This will remove any messages!
		HX_TRACE("There's a message that would've go you!!\r\n");

		/* We do not care about these messages any more */
#ifdef HELIX_FEATURE_NETWORK_USE_SELECT
		// peek doesn't remove wm_paint, this becomes an infinite loop
		if (msg.message == WM_PAINT)
		    DispatchMessage(&msg);
#endif
	    }
	}

	UnlinkUs(this);

	/* If we are in the same thread on which the window was created,
	 * call SendMessage else post a message and Sleep to perform
	 * thread context switch
	 */
	if (m_ulSession == CAsyncNetThread::GetSession())
	{
	    SendMessage(m_hWnd, zm_uDestroyMessage, 0, 0);
	}
	else
	{
	    PostMessage(m_hWnd, zm_uDestroyMessage, 0, 0);
	    Sleep(0);
	}
    }

    return(TRUE);
}

// Inserts us at the head of the list
void 
CAsyncNetThread::LinkUs(CAsyncNetThread* pUs)
{
    HX_ASSERT(pUs);
    if (!pUs)
    {
	return;
    }

    //	Tack us on at the head of the list since order doesn't matter
    pUs->m_pNextNotifier = zm_pSockNotifiers;
    zm_pSockNotifiers = pUs;
}

// Removes us from the list
void 
CAsyncNetThread::UnlinkUs(CAsyncNetThread* pUs)
{
    CAsyncNetThread* pWalk = 0;

    HX_ASSERT(pUs);

    //	Don't try to remove NULL pointers
    if (!pUs)
    {
	return;
    }

    if (zm_pSockNotifiers == pUs)
    {
	zm_pSockNotifiers = pUs->m_pNextNotifier;
	return;
    }
    else
    {
	pWalk = zm_pSockNotifiers;
	while (pWalk && (pWalk->m_pNextNotifier != pUs))
	{
	    pWalk = pWalk->m_pNextNotifier;
	}
	//	Did we find ourselves?
	if (pWalk)
	{
	    //	Ok, link to our successor
	    pWalk->m_pNextNotifier = pUs->m_pNextNotifier;
	}
    }

    return;
}

// Returns correct notifier object for our current session (thread/task)
CAsyncNetThread*
CAsyncNetThread::FindNetThreadNotifier()
{
    ULONG32	    ulSession;
    CAsyncNetThread*    pWalk;

    ulSession = CAsyncNetThread::GetSession();
    pWalk = CAsyncNetThread::zm_pSockNotifiers;

    while (pWalk && pWalk->m_ulSession != ulSession)
    {
	pWalk = pWalk->m_pNextNotifier;
    }
    
    return (pWalk);
}


HX_RESULT	
CAsyncNetThread::AttachSocket(ThreadedConn* pSocket)
{
    void* pVoid = 0;
    HX_RESULT theErr = HXR_OK;

    //	Preliminary checks
    HX_ASSERT(this);
    HX_ASSERT(pSocket);

    //	Bail on bad input
    if (!this || !pSocket)
    {
	theErr = HXR_FAIL;
	goto exit;
    }

    if (!m_ClientSocketsMap->Lookup((void*)pSocket, pVoid))
    {
	pSocket->AddRef();
	m_ClientSocketsMap->SetAt((void*)pSocket, (void*)pSocket);

	//	Bump our client count up
	IncrementSocketsClientCount();
    }
    else
    {
	theErr = HXR_FAIL;
    }

exit:
    return theErr;
}

HX_RESULT	
CAsyncNetThread::DetachSocket(ThreadedConn* pSocket)
{
    HX_RESULT theErr = HXR_OK;

    //	Preliminary checks
    HX_ASSERT(this);
    HX_ASSERT(pSocket);

    //	Bail on bad input
    if (!this || !pSocket)
    {
	theErr = HXR_FAIL;
	goto exit;
    }

    if (m_ClientSocketsMap)
    {
	if (m_ClientSocketsMap->RemoveKey((void*)pSocket))
	{
	    pSocket->Release();
	    DecrementSocketsClientCount();
	}
    }

    CheckClients();

exit:
    return theErr;
}


LRESULT		
CAsyncNetThread::OnAsyncDNS(WPARAM wParam, LPARAM lParam)
{
//    m_pThreadEngineMutex->Lock();

    void* pVoid = 0;
    if (!m_ClientSocketsMap ||
	!m_ClientSocketsMap->Lookup((void*) wParam, pVoid))
    {
//	m_pThreadEngineMutex->Unlock();
    	return TRUE;
    }

    ThreadedConn* pConnection = (ThreadedConn*) pVoid;

    pConnection->OnAsyncDNS((HXBOOL) lParam);
    
//    m_pThreadEngineMutex->Unlock();
    return TRUE;
}

LRESULT		
CAsyncNetThread::OnAsyncConnect(WPARAM wParam, LPARAM lParam)
{
//    m_pThreadEngineMutex->Lock();
    void* pVoid = 0;
    if (!m_ClientSocketsMap ||
	!m_ClientSocketsMap->Lookup((void*) wParam, pVoid))
    {
//	m_pThreadEngineMutex->Unlock();
	return TRUE;
    }

    ThreadedConn* pConnection = (ThreadedConn*) pVoid;

    pConnection->OnConnect((HXBOOL) lParam);
    
//    m_pThreadEngineMutex->Unlock();
    return TRUE;
}

LRESULT		
CAsyncNetThread::OnAsyncRead(WPARAM wParam, LPARAM lParam)
{
//    m_pThreadEngineMutex->Lock();
    void* pVoid = 0;
    if (!m_ClientSocketsMap ||
	!m_ClientSocketsMap->Lookup((void*) wParam, pVoid))
    {
//	m_pThreadEngineMutex->Unlock();
	return TRUE;
    }

    ThreadedConn* pConnection = (ThreadedConn*) pVoid;

    pConnection->OnReadNotification();
    
//    m_pThreadEngineMutex->Unlock();
    return TRUE;
}

LRESULT		
CAsyncNetThread::OnAsyncWrite(WPARAM wParam, LPARAM lParam)
{
//    m_pThreadEngineMutex->Lock();
    void* pVoid = 0;
    if (!m_ClientSocketsMap ||
	!m_ClientSocketsMap->Lookup((void*) wParam, pVoid))
    {
//	m_pThreadEngineMutex->Unlock();
	return TRUE;
    }

    ThreadedConn* pConnection = (ThreadedConn*) pVoid;

    pConnection->OnWriteNotification();
    
//    m_pThreadEngineMutex->Unlock();
    return TRUE;
}



LRESULT		
CAsyncNetThread::OnAsyncAccept(WPARAM wParam, LPARAM lParam)
{
//    m_pThreadEngineMutex->Lock();
    void* pVoid = 0;
    if (!m_ClientSocketsMap ||
	!m_ClientSocketsMap->Lookup((void*) wParam, pVoid))
    {
//	m_pThreadEngineMutex->Unlock();
	return TRUE;
    }

    ThreadedConn* pConnection = (ThreadedConn*) pVoid;

    pConnection->OnAcceptNotification();
    
//    m_pThreadEngineMutex->Unlock();
    return TRUE;
}
