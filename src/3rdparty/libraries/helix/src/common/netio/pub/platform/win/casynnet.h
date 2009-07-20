/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: casynnet.h,v 1.5 2007/04/13 19:50:28 ping Exp $
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

#if !defined( __CASYNNET_H )
#define	__CASYNNET_H

#include "hxtypes.h"
#include "conn.h"
#include "platform/win/win_net.h"
#include "hxthread.h"
#include "hxassert.h"

#define		CLASS_NAME_CASYNCSOCK		"HX_Internal_Sock_Async"

//	Predeclare these guys so we don't have to include their header files
class	win_net;
class	CHXMapPtrToPtr;

class DNS_REQUEST
{
public:
    DNS_REQUEST();
    ~DNS_REQUEST();

    win_net*	fpSock;
    char*	lpHostName;
    char*	lpBuffer;
    int		cbBufferSize;
};

class CAsyncSockN
{
public:
    //	Client Sockets call this guy to get a Notifier, either created or 
    //	an already created socket notifier.
    //	Guaranteed to return either NULL or a valid Notifier
    static CAsyncSockN* GetCAsyncSockNotifier(HINSTANCE hInst, 
					      HXBOOL Create = FALSE);

    //	No reason why these should fail unless win_net is invalid
    HXBOOL		DoAsyncDNS(win_net* fpSock, LPCSTR lpHostName, 
				   LPSTR pBuff, int cbBuffLen);
    HXBOOL		DoAsyncSelect(win_net* fpSock);
    HXBOOL		CancelSelect(win_net* fpSock);

protected:
    /*
     *	Class Private Utility Methods
     */
    static ULONG32  GetSession();

    //	Hidden constructor
    CAsyncSockN(HINSTANCE hInst);

    //	Internal create helper
    HXBOOL	    Create();

    //	Hidden Destructor
    ~CAsyncSockN();

    //	Validates the object
    HXBOOL	    IsValid()
    {
	return(m_bValid);
    }

    //	Decrements our client count
    void DecrementHandlesClientCount()
    {
	HX_ASSERT(m_cbNumHandlesClients > 0);
	m_cbNumHandlesClients--;
    }

    void DecrementSocketsClientCount()
    {
	HX_ASSERT(m_cbNumSocketsClients > 0);
	m_cbNumSocketsClients--;
    }

    void IncrementSocketsClientCount()
    {
	m_cbNumSocketsClients++;
    }

    void IncrementHandlesClientCount()
    {
	m_cbNumHandlesClients++;
    }
    
    //	These are the definitions of the messages our WndProc handles
    enum
    {
	//  Internally used message for introducing latency between deletes
	PWM_CHECK_CLIENTS = WM_USER + 100,

	//  AsyncSelect Notification for one of our clients
	PWM_ASYNC_SELECT,

	//  Async DNS Notification
	PWM_ASYNC_DNS,
    };

    //	WndProc for our notifier	
    static LRESULT CALLBACK AsyncNotifyProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    //	Dispatched methods from the WndProc
    LRESULT		OnAsyncDNS(WPARAM wParam, LPARAM lParam);
    LRESULT		OnAsyncSelect(WPARAM wParam, LPARAM lParam);

    // cleanup if class is not used by any clients
    LRESULT		CheckClients();

    //	Linked list of US management functions
    //	Inserts us at the head of the list
    static void	LinkUs(CAsyncSockN* pUs);

    //	Removes us from the list
    static void	UnlinkUs(CAsyncSockN* pUs);

    //	Finds a notifier for the current thread in the linked list
    static CAsyncSockN* FindSockNotifier();

    /*
     *	Class private data members
     */
    //	Don't try to register multiple times
    static HXBOOL		zm_bClassRegistered;

    //	Points to the head of a list of CAsyncSockN objects so we don't 
    //	create one for each win_net object (only per thread or per task)
    static CAsyncSockN*	zm_pSockNotifiers;

    //	Store the link in a nonstatic member
    CAsyncSockN*	m_pNextNotifier;

    HWND		m_hWnd;
    HXBOOL		m_bValid;

    //	Win32 this is the thread ID, Win16 this is the current task
    ULONG32		m_ulSession;

    HINSTANCE		m_hInst;


    //	Our WndProc recieves either Async DNS handles or Socket handles
    //	and needs to convert these to the win_net object they correspond to.

    //	Map that holds our client objects mapped from Async Handles
    CHXMapPtrToPtr*	m_ClientHandlesMap;
    int			m_cbNumHandlesClients;

    //	Map that holds our client objects mapped from Socket Handles
    CHXMapPtrToPtr*	m_ClientSocketsMap;
    int			m_cbNumSocketsClients;
    HXBOOL		m_bInCancelMode;

    HXBOOL		ProcessDNSQueue();
};

class SockGlobals
{
public:
    HXBOOL		m_bWinSock2Suck;
    CHXSimpleList	m_DNSQueue;

    SockGlobals();
    ~SockGlobals();
};

extern SockGlobals sockGlobals;

#endif	//	__CASYNNET_H
