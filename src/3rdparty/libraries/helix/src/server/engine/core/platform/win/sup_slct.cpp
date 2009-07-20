/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sup_slct.cpp,v 1.3 2003/01/25 00:06:19 bgoldfarb Exp $ 
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
 *  SuperSelector:  This object implements a better select (SuperSelect).  It is
 *  better because another thread can signal a thread waiting on this select
 *  by calling KillSelect.  Be aware that any sockets registered with this object
 *  will be set to non-blocking.
 */

#if defined USE_WINSOCK1

#include <stdio.h>
#include "sup_slct.h"
#include "debug.h"

SuperSelector::RegisterProtector SuperSelector::m_rp = RegisterProtector();
BOOL SuperSelector::m_bClassBeenRegistered = FALSE;

/*
 * SuperSelector constructor.
 */

SuperSelector::SuperSelector() :
m_hWnd(NULL),
m_timer(1)
{
}

/*
 * SuperSelector destructor.
 */

SuperSelector::~SuperSelector(){
    if(m_hWnd) {
	DestroyWindow(m_hWnd);
	m_hWnd = NULL;
    }
}

/* void SuperSelector::init()
 *
 * Creates and initializes the window to recieve the asyncronous
 * messages.
 */



int
SuperSelector::Init() {
    WNDCLASS wndclass;

    wndclass.style = NULL;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = NULL;
    wndclass.hIcon = NULL;
    wndclass.hCursor = NULL;
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName = (LPSTR) "";
    wndclass.lpszClassName = (LPSTR) "HX_SuperSelector_wnd";

    m_rp.Enter();
    if(m_bClassBeenRegistered == FALSE)
    {
	if(!RegisterClass(&wndclass)) {
	    DWORD dw = GetLastError();
	    if(dw && dw != ERROR_CLASS_ALREADY_EXISTS) {
		m_rp.Leave();;
		PANIC(("SuperSelector could not register wnd class!\n"));
		return 0;
	    }
	}
	m_bClassBeenRegistered = TRUE;
    }
    m_rp.Leave();

    m_hWnd = CreateWindow(wndclass.lpszClassName, //classname
	"", //windowname
	WS_DISABLED, //style; this should prevent ui messages.
	0, //x
	0, //y
	0, //width
	0, //height
	NULL, //parent
	NULL, //hMenu
	NULL, //hInstance
	NULL);
    if(!m_hWnd) {
	PANIC(("SuperSelector could not CreateWindow!\n"));
	return 0;
    }
    return 1;
}

/* LRESULT CALLBACK SuperSelector::WndProc(HWND, UINT, WPARAM, LPARAM)
 *
 * This is a dummy WindowProc.  Its only purpose is to return TRUE while
 * create window is doing its stuff.  This WindowProc does not even get
 * messages from our message pump, as we handle them directly from 
 * GetMessage.
 */

LRESULT CALLBACK
SuperSelector::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg)
    {
	case WM_CREATE:
	    return 0;
	case WM_NCCREATE:
	    return TRUE;
	case WM_GETMINMAXINFO:
	    return 0;
	case WM_NCCALCSIZE:
	    return 0;
    }
    return TRUE;
}

/* int SuperSelector::RegisterSocket(SOCKET s, long notification)
 *
 * Just calls WSAAsyncSelect with our window handle.
 */

int
SuperSelector::RegisterSocket(SOCKET s, long notification) {
    return WSAAsyncSelect(s, m_hWnd, WM_SUPERSELECTOR_SOCK, notification);
}

/* int SuperSelector::UnregisterSocket(SOCKET s)
 * 
 * Calls WSAAsyncSelect with our window handle and no lEvent.  This will keep
 * the messages from getting sent to our message queue.
 */

int
SuperSelector::UnregisterSocket(SOCKET s) {
    return WSAAsyncSelect(s, m_hWnd, 0, 0);
}

/* SOCKET SuperSelector::SuperSelect(UINT tout, SOCKET *ps, long* pState)
 * 
 * Works like select, except for
 * 1) The sockets must be registered individually
 *    with RegisterSocket
 * and
 * 2) You can break out of it by posting a WM_EXITSELECT to the our window
 *    handle (which you can get through GetWindow()), or calling KillSelect().
 *
 * Parameters: UINT tout: timeout in millis or INFINITE_TOUT.
 *             SOCKET *ps: optional SOCKET * to get the socket that trigered the
 *                     exit.  Only valid if return is SOCKET_EXIT.
 *              long *pState: state of the exiting socket. FD_ACCEPT, 
 *              FD_CONNECT, FD_READ, FD_WRITE, etc.
 *
 * Returns: one of TIMEOUT_EXIT, SIGNALED_EXIT, SOCKET_EXIT.
 */


long
SuperSelector::SuperSelect(UINT tout, SOCKET *ps, long* pState) {
    MSG msg;
    LONG ret;
    BOOL bContinue = TRUE;
    //set up a timer so that we can time out.
    //kill this is we exit for another reason.
    if(tout != INFINITE_TOUT) {
	SetTimer(m_hWnd, m_timer, tout, NULL);
    }

    while(bContinue) {
	GetMessage(&msg, m_hWnd, 0, 0);
	switch(msg.message) {
	//Winsock related message
	case WM_SUPERSELECTOR_SOCK:
	    if(pState) {
		//set the state to the reason we are being notified.
		*pState = WSAGETSELECTEVENT(msg.lParam);
	    }
	    if(ps) {
		// wParam is the SOCKET.
		*ps = msg.wParam;
	    }
	    ret = SOCKET_EXIT;
	    bContinue = FALSE;
	    break;
	//signaled exit.
	case WM_EXITSELECT:
	    ret = SIGNALED_EXIT;
	    bContinue = FALSE;
	    break;
	//timer went off.
	case WM_TIMER:
	    ret = TIMEOUT_EXIT;
	    bContinue = FALSE;
	    break;
	}
    }
    KillTimer(m_hWnd, m_timer);
    return ret;
}

/* BOOL SuperSelect::KillSelect()
 *
 * Will cause SuperSelector to drop out of a call to SuperSelect.
 */

BOOL
SuperSelector::KillSelect() {
    return PostMessage(m_hWnd, WM_EXITSELECT, 0, 0);
}

#endif
