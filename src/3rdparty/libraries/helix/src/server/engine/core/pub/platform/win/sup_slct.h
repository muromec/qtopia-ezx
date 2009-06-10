/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sup_slct.h,v 1.4 2004/05/13 18:57:48 tmarshall Exp $ 
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

#ifndef _SUP_SLCT_H_
#define _SUP_SLCT_H_

#include "hlxclib/windows.h"

#define WM_SUPERSELECTOR_SOCK  WM_USER +1
#define WM_EXITSELECT WM_USER +2

#define TIMEOUT_EXIT    -1
#define SIGNALED_EXIT	-2
#define SOCKET_EXIT     -3
#define KEEP_GOING	-4 //not used anymore

#define INFINITE_TOUT -1

class SuperSelector
{
public:
		SuperSelector();
		~SuperSelector();

    int		Init();
    static LRESULT CALLBACK
		WndProc(HWND, UINT, WPARAM, LPARAM);
    int		RegisterSocket(SOCKET, long);
    int		UnregisterSocket(SOCKET);
    HWND	GetWindow() {return m_hWnd;}
    long	SuperSelect(UINT tout, SOCKET *ps = NULL, long *pState = NULL);
    BOOL	KillSelect();


protected:
    class RegisterProtector
    {	
    public:
	RegisterProtector()
	{
	    InitializeCriticalSection(&m_cs);
	}
	~RegisterProtector()
	{
	    DeleteCriticalSection(&m_cs);
	}
	CRITICAL_SECTION m_cs;
	void Enter()
	{
	    EnterCriticalSection(&m_cs);
	}
	void Leave()
	{
	    LeaveCriticalSection(&m_cs);
	}
    };
    HWND			m_hWnd;
    UINT			m_timer;
    static RegisterProtector	m_rp;
    static BOOL			m_bClassBeenRegistered;
};

#endif //_SUP_SLCT_H_

#endif //USE_WINSOCK1
