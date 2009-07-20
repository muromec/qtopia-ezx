/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: casynnet.cpp,v 1.10 2007/07/06 20:43:57 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hlxclib/windows.h"	//	For Window-isms
#include "hxcom.h"
#include "hxslist.h"
#include "platform/win/sock.h"
#include "platform/win/hxsock.h"   // For Socket related constants and macros
#include "platform/win/casynnet.h"	//	Our definition
#include "hxmap.h"	//	Map object
#include "debugout.h"	//	DEBUGOUTSTR()
#include "hxassert.h"	//	HX_ASSERT()
#include "hxstrutl.h"

#include "hxheap.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif									 

/* Constant Definitions */
#define	WND_CLASS_BASE		"HX_Internal_Sock_Async"
#define	MAX_WND_CLASS_LENGTH	50
#define	OFFSET_THIS		0

/* Class Static Definitions */
HXBOOL		CAsyncSockN::zm_bClassRegistered    = FALSE;
CAsyncSockN*	CAsyncSockN::zm_pSockNotifiers	    = NULL;

SockGlobals sockGlobals;

/* Class Public Methods */

// Either creates a socket notifier, or returns a pointer
// to the socket notifier that should be used for this instance.
CAsyncSockN *CAsyncSockN::GetCAsyncSockNotifier( HINSTANCE hInst , HXBOOL Create)
{
    CAsyncSockN*    pThis = 0;

    //	Does our current session already have a net notifier?
    if (!(pThis = FindSockNotifier()) && Create)
    {
	//	No, then create one
	pThis = new CAsyncSockN(hInst);
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
    //	return the results of our labor
    return (pThis);
}

//	Our client win_net object calls this method to
//	start an async DNS request.
HXBOOL CAsyncSockN::DoAsyncDNS( win_net *fpSock, LPCSTR lpHostName, LPSTR pBuff, int cbBuffSize )
{
    //	Preliminary checks
    HX_ASSERT(this);
    HX_ASSERT(fpSock);
    HX_ASSERT(m_hWnd);
    HX_ASSERT(lpHostName && *lpHostName);

    //	Bail on bad input
    if (!this || !fpSock || !lpHostName || !*lpHostName)
    {
	return(FALSE);
    }

    DNS_REQUEST* pDNSRequest = new DNS_REQUEST;
    pDNSRequest->fpSock = fpSock;

    if (lpHostName)
    {
	pDNSRequest->lpHostName = new CHAR[strlen(lpHostName)+1];
	memset(pDNSRequest->lpHostName, 0, strlen(lpHostName)+1);

	strcpy(pDNSRequest->lpHostName, lpHostName); /* Flawfinder: ignore */
    }
    
    pDNSRequest->lpBuffer = pBuff;
    pDNSRequest->cbBufferSize = cbBuffSize;

    sockGlobals.m_DNSQueue.AddTail(pDNSRequest);
    
    return ProcessDNSQueue();
}

HXBOOL CAsyncSockN::ProcessDNSQueue(void)
{
    HXBOOL	    bResult = TRUE;
    DNS_REQUEST*    pDNSRequest = NULL;
    HANDLE	    hAsyncHandle = NULL;
    int		    nCount = 0;

    if (sockGlobals.m_DNSQueue.GetCount())
    {
	if (sockGlobals.m_bWinSock2Suck)
	{
	    nCount = m_ClientHandlesMap->GetCount();

	    if (!nCount)
	    {
		pDNSRequest = (DNS_REQUEST*)sockGlobals.m_DNSQueue.GetHead();
	    }
	    else
	    {
		goto cleanup;
	    }
	}
	else
	{
	    pDNSRequest = (DNS_REQUEST*)sockGlobals.m_DNSQueue.RemoveHead();
	}

	// Start the async DNS
	hAsyncHandle = sockObj->HXWSAAsyncGetHostByName(m_hWnd, 
							PWM_ASYNC_DNS, 
							pDNSRequest->lpHostName, 
							pDNSRequest->lpBuffer, 
							pDNSRequest->cbBufferSize);

	// Create a mapping from the async handle to the win_net object pointer
	m_ClientHandlesMap->SetAt((void*)hAsyncHandle, pDNSRequest->fpSock);

	// You shouldn't already have an async operation active on this socket!
	HX_ASSERT(pDNSRequest->fpSock->m_hAsyncHandle == NULL);

	// Remember this in case we need to cancel!
	pDNSRequest->fpSock->m_hAsyncHandle = hAsyncHandle;

	// Bump our client count up
	IncrementHandlesClientCount();
    }

cleanup:

    if (!sockGlobals.m_bWinSock2Suck)
    {
	HX_DELETE(pDNSRequest);
    }

    return bResult;
}
   
//	Our client win_net object calls this method to 
//	start receiving async select notifications
//	on a socket.
HXBOOL CAsyncSockN::DoAsyncSelect(win_net* fpSock)
{
    SOCKET	theSocket;

    //	Preliminary checks
    HX_ASSERT(this);
    HX_ASSERT(fpSock);

    //	Bail on bad input
    if (!this || !fpSock)
    {
	return(FALSE);
    }

    //	Get and verify we have a good socket handle
    theSocket = fpSock->get_sock();
    HX_ASSERT(theSocket != INVALID_SOCKET);

    //	Request async Notifications on the socket
    long lEvent = (FD_CONNECT | FD_READ | FD_CLOSE) ;
#ifndef HELIX_FEATURE_NETWORK_USE_SELECT
    lEvent |= (FD_WRITE | FD_ACCEPT);
#endif
    sockObj->HXWSAAsyncSelect(theSocket, m_hWnd, PWM_ASYNC_SELECT, lEvent);

    //	Create a mapping from socket handle to win_net object pointer
    m_ClientSocketsMap->SetAt((void*)theSocket, fpSock);

    //	Bump our client count up
    IncrementSocketsClientCount();
    return (TRUE);
}

//	Our client win_net object calls this method to 
//	stop receiving async select notifications
//	on a socket.
HXBOOL CAsyncSockN::CancelSelect(win_net* fpSock)
{
    SOCKET	    theSocket;
    DNS_REQUEST*    pDNSRequest = NULL;
    LISTPOSITION    lPos;

    //	Preliminary checks
    HX_ASSERT(this);
    HX_ASSERT(fpSock);

    //	Bail on bad input
    if (!this || !fpSock)
    {
	return(FALSE);
    }

    m_bInCancelMode = TRUE;

    // If we haven't returned from an Asyn operation like GetHostByName
    // then we need to tell Winsock to cancel the opertation!
    HANDLE hAsyncHandle = fpSock->m_hAsyncHandle;
    if (hAsyncHandle)
    {
	sockObj->HXWSACancelAsyncRequest(hAsyncHandle);
	fpSock->m_hAsyncHandle = NULL;

	// If we've canceled the async DNS is done, 
	// then we can forget the Async handle
	if (m_ClientHandlesMap->RemoveKey((void*)hAsyncHandle))
	{
	    DecrementHandlesClientCount();
	}
    }

    if (sockGlobals.m_bWinSock2Suck)
    {
	lPos = sockGlobals.m_DNSQueue.GetHeadPosition();

	while (lPos)
	{
	    pDNSRequest = (DNS_REQUEST *)sockGlobals.m_DNSQueue.GetAt(lPos);

	    if (pDNSRequest->fpSock == fpSock)
	    {
		sockGlobals.m_DNSQueue.RemoveAt(lPos);
		HX_DELETE(pDNSRequest);
		break;
	    }

	    sockGlobals.m_DNSQueue.GetNext(lPos);
	}
    }

    //	Get and verify we have a good socket handle
    theSocket = fpSock->get_sock();
    
//    HX_ASSERT(theSocket != INVALID_SOCKET);
    /* This may actually happen since we do not check for CONN_CLOSED
     * condition in win_net any more
     */
    if (theSocket == INVALID_SOCKET)
    {
	m_bInCancelMode = FALSE;
	return FALSE;
    }

    //	Turn off async Notifications on the socket
    sockObj->HXWSAAsyncSelect(theSocket, m_hWnd, 0, 0);

    // According to the Winsock help, just cause we cancel
    // the notifications, doesn't mean we still won't get notified
    // of something that has already been posted to our window,
    // so... let's flush our message queue...
    //
    // HACK ATTACK: Notice that both of these message pumps
    // are needed. If you run a debug build or a retail build
    // on some machines without the two message pumps, you
    // won't have any trouble. But on some machines having only
    // this last message pump will cause a GPF.
    MSG msg;
    while (m_hWnd && PeekMessage(&msg, m_hWnd,0,0,PM_REMOVE))
    {
	// This will remove any messages!
	HX_TRACE("There's a message that would've go you!!\r\n");

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
	    // Even though we want them removed, we also want to act on them.
	    DispatchMessage(&msg);
	}
    }

    if (m_ClientSocketsMap)
    {
	//	Delete the mapping from socket handle to win_net object pointer
	if (m_ClientSocketsMap->RemoveKey((void*)theSocket))
	{
	    //	Decrement our client count and conditionally
	    //	delete ourselves.
	    DecrementSocketsClientCount();
	}
    }

    m_bInCancelMode = FALSE;

    // check to see if we are in use
    CheckClients();
    return (TRUE);
}


/*
 *	Private Class Methods
 */

//	Static method
//	Computes a session id that should be thread unique
//	In Win32 we have a thread ID
//	In Win16 we have the task (single threaded)
ULONG32 CAsyncSockN::GetSession()
{
#if defined( _WIN32 )
	return (GetCurrentThreadId());
#else
	return ((ULONG32)(LPSTR)GetCurrentTask());
#endif
}

//	Hidden Constructor, called by GetAsyncNotifier() if there
//	is no Socket Notifier for the current session
CAsyncSockN::CAsyncSockN(HINSTANCE hInst)
    : m_hWnd(NULL)
    , m_bValid(FALSE)
    , m_hInst(hInst)
    , m_cbNumHandlesClients(0)
    , m_cbNumSocketsClients(0)
    , m_bInCancelMode(FALSE)
    , m_pNextNotifier(0)
{
    //	Validate input
    HX_ASSERT(this);
    // assert( hInst );
    if (!this)
    {
	return;
    }

    m_ClientHandlesMap = new CHXMapPtrToPtr;
    m_ClientSocketsMap = new CHXMapPtrToPtr;

    m_ulSession = CAsyncSockN::GetSession();

    m_bValid = Create();
}

//	Workhorse construction method
HXBOOL CAsyncSockN::Create()
{
    char szClassName[MAX_WND_CLASS_LENGTH]; /* Flawfinder: ignore */
    SafeSprintf(szClassName, MAX_WND_CLASS_LENGTH, "%s%d", WND_CLASS_BASE, &zm_bClassRegistered);

    if (!zm_bClassRegistered)
    {
	WNDCLASS internalClass;
    
	//	First register our window class                                  
	internalClass.style 		= 0;
	internalClass.lpfnWndProc 	= CAsyncSockN::AsyncNotifyProc;
	internalClass.cbClsExtra	= 0;
	internalClass.cbWndExtra	= sizeof(this);
	internalClass.hInstance		= m_hInst; // Use app's instance
	internalClass.hIcon		= 0;
	internalClass.hCursor		= 0;
	internalClass.hbrBackground	= 0;
	internalClass.lpszMenuName	= NULL;
	internalClass.lpszClassName	= szClassName;
	
	zm_bClassRegistered = RegisterClass(&internalClass);
	
	if(!zm_bClassRegistered)
	{
	    UnregisterClass(szClassName, m_hInst);
	    zm_bClassRegistered = RegisterClass(&internalClass);
	}
    }

    //	Now create an instance of the window	
    m_hWnd = CreateWindow(szClassName, "RealAudio Internal Messages", 
		WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, m_hInst, this);
 
    if (!m_hWnd)
    {
	return (FALSE);
    }

    return (TRUE);
}

//	Destructor: Destroys the window and unregisters the class if necessary
CAsyncSockN::~CAsyncSockN()
{
    //	Validate input
    HX_ASSERT(this);
    if (!this)
    {
	return;
    }

    m_bValid = FALSE;
    // UnlinkUs( this );	
    m_hWnd = NULL;
    m_hInst = NULL;

    if(m_ClientHandlesMap)
    {
	delete m_ClientHandlesMap;
	m_ClientHandlesMap = NULL;
	m_cbNumHandlesClients = 0;
    }

    if(m_ClientSocketsMap)
    {
	delete m_ClientSocketsMap;
	m_ClientSocketsMap = NULL;
	m_cbNumSocketsClients = 0;
    }
}

//	This is the WndProc that handles async notifications on behalf of the client
//	win_net objects.
LRESULT 
CAsyncSockN::AsyncNotifyProc(HWND hWnd, UINT msg, 
					      WPARAM wParam, LPARAM lParam)
{
    CAsyncSockN*    pThis = 0;


    //	Make ourselves look like a non-static member function dig the this
    //	pointer out of the window.
    if (msg == WM_NCCREATE)
    {
	CREATESTRUCT* lpCreate = 0;

	// Set our this pointer, so our WndProc can find us again
	lpCreate = (CREATESTRUCT FAR*) lParam;
	pThis = (CAsyncSockN*) lpCreate->lpCreateParams;
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
	//	Note:	This MUST be a static function
	//	However we make it appear to be a member function
	//	By imbedding a this pointer in the window object
	pThis = (CAsyncSockN*) (LPHANDLE)GetWindowLong(hWnd, OFFSET_THIS);
    }

    if (pThis != NULL)
    {
	switch (msg)
	{
	    case PWM_ASYNC_SELECT:
		return (pThis->OnAsyncSelect(wParam, lParam));

	    case PWM_ASYNC_DNS:
		return (pThis->OnAsyncDNS(wParam, lParam));

	    default:
		break;
	}
    }

    return (DefWindowProc(hWnd, msg, wParam, lParam));
}

LRESULT CAsyncSockN::CheckClients()
{
    ProcessDNSQueue();
        
    if (!m_bInCancelMode && !m_cbNumHandlesClients && !m_cbNumSocketsClients)
    {
	UnlinkUs(this);
	DestroyWindow(m_hWnd);
	delete this;
    }

    return(TRUE);
}

//	Message handler for PWM_ASYNC_DNS
//	Notifies the client win_net object that DNS has completed or
//	returned w/ an error.
LRESULT CAsyncSockN::OnAsyncDNS(WPARAM wParam, LPARAM lParam)
{
    void*	    handle	= (void*) (HANDLE) wParam;
    void*	    pVoid	= 0;
    win_net*	    pClient	= 0;
    DNS_REQUEST*    pDNSRequest	= NULL;

    // Bail if the client map has been previously deleted, this
    // is a sure sign of us already being cleaned up!
    if(!m_ClientHandlesMap)
    {
	return(TRUE);
    }

    DEBUGOUTSTR( "AsyncDNS come back\r\n" );

    // Based on the handle returned, find our client!
    // Note it might be missing from the map if we canceled
    // the DNS!
    if (m_ClientHandlesMap->Lookup(handle, pVoid))
    {
	*((ULONG32 *)&pClient) = (ULONG32)pVoid;

	// We should already have an async operation active on this socket!
#ifdef _DEBUG
	if ((void *)pClient->m_hAsyncHandle != handle)
	{
	    char szMessage[256]; /* Flawfinder: ignore */
	    sprintf(szMessage,"pClient->m_hAsyncHandle = %#lx\nhandle = %#lx", /* Flawfinder: ignore */
				pClient->m_hAsyncHandle,
				handle
			    );
	    MessageBox(NULL,szMessage,"Oops!",MB_OK);
	}
	HX_ASSERT((void *)pClient->m_hAsyncHandle == handle);
#endif

	// Tell win_net object to forget the Async handle
	pClient->m_hAsyncHandle = NULL;

	pClient->CB_DNSComplete(!WSAGETASYNCERROR(lParam));

	// If async DNS is done, then we can forget the Async handle
	if (m_ClientHandlesMap->RemoveKey(handle))
	{
	    DecrementHandlesClientCount();
	}
    }

#ifdef _DEBUG
    if (m_ClientHandlesMap->GetCount() != m_cbNumHandlesClients)
    {
	char szMessage[256]; /* Flawfinder: ignore */
	sprintf(szMessage,"m_ClientHandlesMap->GetCount() = %d\nm_cbNumHandlesClients = %d", /* Flawfinder: ignore */
			    m_ClientHandlesMap->GetCount(),
			    m_cbNumHandlesClients
			);
	MessageBox(NULL,szMessage,"Oops!",MB_OK);
    }
    HX_ASSERT(m_ClientHandlesMap->GetCount() == m_cbNumHandlesClients);
#endif

    if (sockGlobals.m_bWinSock2Suck)
    {
	pDNSRequest = (DNS_REQUEST*) sockGlobals.m_DNSQueue.RemoveHead();
	HX_DELETE(pDNSRequest);
    }

    // check to see if we are still in use.
    CheckClients();
    return (TRUE);
}

//	Message handler for PWM_ASYNC_SELECT
//	Notifies the win_net client when the corresponding socket
//	is ready to read/write/ or has connected.
LRESULT CAsyncSockN::OnAsyncSelect(WPARAM wParam, LPARAM lParam)
{
    LPVOID		pVoid;
    UINT16		theEvent;
    UINT16		theError;
    SOCKET		theSocket;
    win_net*		pClient = NULL;

    theSocket = (SOCKET) wParam;
    theEvent = WSAGETSELECTEVENT(lParam);
    theError = WSAGETSELECTERROR(lParam);

    //	Find out the win_net object we want to talk to
    if(m_ClientSocketsMap->Lookup((void*) theSocket, pVoid))
    {
	*((ULONG32 *)&pClient) = (ULONG32) pVoid;
    }

    if (!pClient)
    {
	DEBUGOUTSTR( "Error OnAsyncSelect() no win_net object found\r\n" );
	return(TRUE);
    }

    if (theError)
    {
	char	acError[100]; /* Flawfinder: ignore */

	wsprintf(acError, "Got error %d from Winsock\r\n", theError);
	DEBUGOUTSTR(acError);
    }

    switch (theEvent)
    {
    case FD_WRITE:
	DEBUGOUTSTR("Got AsyncWrite notification\r\n");
	pClient->CB_ReadWriteNotification(theEvent);
	break;

    case FD_READ:
#ifndef _DEMPSEY
	// Noisy debug output
	DEBUGOUTSTR("Got AsyncRead notification\r\n");
#endif // _DEMPSEY
	pClient->CB_ReadWriteNotification(theEvent);
	break;

    case FD_CONNECT:
	DEBUGOUTSTR("Got AsyncConnect notification\r\n");
	pClient->CB_ConnectionComplete(!theError);
	break;

    case FD_CLOSE:
	DEBUGOUTSTR("Got AsyncClose notification\r\n");
	pClient->CB_CloseNotification();
	break;
    
    case FD_ACCEPT:
	DEBUGOUTSTR("Got AsyncAccept notification\r\n");
	pClient->CB_AcceptNotification();
	break;
    }

    return (TRUE);
}

//	Static method
//	Inserts us at the head of the list
void CAsyncSockN::LinkUs(CAsyncSockN* pUs)
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

//	Static method
//	Removes us from the list
void CAsyncSockN::UnlinkUs(CAsyncSockN* pUs)
{
    CAsyncSockN* pWalk = 0;

    HX_ASSERT(pUs);

    //	Don't try to remove NULL pointers
    if (!pUs)
    {
	return;
    }

    //	If the desired node is at the head then it's simple
    if (zm_pSockNotifiers == pUs)
    {
	zm_pSockNotifiers = pUs->m_pNextNotifier;
	return;
    }
    //	Otherwise we have to walk the list till we find our node
    else
    {
	//	iterate till we find ourselves or reach the end of the list
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

//	Static method
//	Returns correct notifier object for our current session (thread/task)
CAsyncSockN *CAsyncSockN::FindSockNotifier()
{
    ULONG32	    ulSession;
    CAsyncSockN*    pWalk;

    ulSession = CAsyncSockN::GetSession();
    pWalk = CAsyncSockN::zm_pSockNotifiers;

    while (pWalk && pWalk->m_ulSession != ulSession)
    {
	pWalk = pWalk->m_pNextNotifier;
    }
    
    return (pWalk);
}

DNS_REQUEST::DNS_REQUEST()
{
    fpSock = NULL;
    lpHostName = NULL;
    lpBuffer = NULL;
    cbBufferSize = 0;
}

DNS_REQUEST::~DNS_REQUEST()
{
    HX_VECTOR_DELETE(lpHostName);
}

SockGlobals::SockGlobals()
{
}

SockGlobals::~SockGlobals()
{
    while (m_DNSQueue.GetCount())
    {
	DNS_REQUEST* pDNSRequest = (DNS_REQUEST*) m_DNSQueue.RemoveHead();
	HX_DELETE(pDNSRequest);
    }
    m_DNSQueue.RemoveAll();
}

