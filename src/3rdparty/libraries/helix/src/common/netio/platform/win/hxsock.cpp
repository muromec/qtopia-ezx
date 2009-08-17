/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsock.cpp,v 1.6 2005/03/14 19:36:36 bobclark Exp $
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
#include "hxassert.h"
#include "hlxclib/windows.h"
#include "platform/win/sock.h"
#include "platform/win/hxsock.h"
#include "hxheap.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif									 

static FARPROC HXGetProcAddress( HMODULE hModule, const char* lpProcName)
{
    return GetProcAddress(hModule, OS_STRING(lpProcName));
}

// Create one object that is referenced throughout the system.
CHXSock*	sockObj = NULL;

CHXSock::CHXSock() :
	m_iInited( NULL ),
	m_hLib( NULL ),
	_hxaccept( NULL ),
	_hxbind( NULL ),
	_hxclosesocket( NULL ),
	_hxconnect( NULL ),
	_hxioctlsocket( NULL ),
	_hxgetpeername( NULL ),
	_hxgetsockname( NULL ),
	_hxgetsockopt( NULL ),
	_hxhtonl( NULL ),
	_hxhtons( NULL ),
	_hxinet_addr( NULL ),
	_hxinet_ntoa( NULL ),
	_hxlisten( NULL ),
	_hxntohl( NULL ),
	_hxntohs( NULL ),
	_hxrecv( NULL ),
	_hxrecvfrom( NULL ),
	_hxselect( NULL ),
	_hxsend( NULL ),
	_hxsendto( NULL ),
	_hxsetsockopt( NULL ),
	_hxshutdown( NULL ),
	_hxsocket( NULL ),
	
	/* Database function prototypes */
	_hxgethostbyaddr( NULL ),
	_hxgethostbyname( NULL ),
	_hxgethostname( NULL ),
#ifndef _WINCE
	_hxgetservbyport( NULL ),
	_hxgetservbyname( NULL ),
	_hxgetprotobynumber( NULL ),
	_hxgetprotobyname( NULL ),
#endif /* _WINCE */

	/* Microsoft Windows Extension function prototypes */
	_hxWSAStartup( NULL ),
	_hxWSACleanup( NULL ),
	_hxWSASetLastError( NULL ),
	_hxWSAGetLastError( NULL ),

#ifndef _WINCE
	_hxWSAIsBlocking( NULL ),
	_hxWSAUnhookBlockingHook( NULL ),
	_hxWSASetBlockingHook( NULL ),
	_hxWSACancelBlockingCall( NULL ),
	_hxWSAAsyncGetServByName( NULL ),
	_hxWSAAsyncGetServByPort( NULL ),
	_hxWSAAsyncGetProtoByName( NULL ),
	_hxWSAAsyncGetProtoByNumber( NULL ),
	_hxWSAAsyncGetHostByName( NULL ),
	_hxWSAAsyncGetHostByAddr( NULL ),
	_hxWSACancelAsyncRequest( NULL ),
	_hxWSAAsyncSelect( NULL ),
#endif /* _WINCE */
	_hxWSAFDIsSet( NULL ),
	m_nVersion(0)

{
	//	This is basically a do nothing but init constructor.

	//	If the load fails we can still come up, but before
	//	we try to play any network resources we'll need to
	//	load and init.
    LoadWinsock();
}

CHXSock::~CHXSock()
{
    UnloadWinsock();
}

HXBOOL CHXSock::LoadWinsock( void )
{
	char	*pcModule;

	if (m_hLib && m_iInited)
	{
		return( TRUE );
	}

	if (m_hLib)
	{
		FreeLibrary( m_hLib );
		m_hLib = (HINSTANCE)0;
	}

#if defined(WIN32_PLATFORM_PSPC)
	pcModule = "WINSOCK.DLL";
	m_hLib = LoadLibrary( OS_STRING(pcModule) );
#elif defined( _WIN32 )
	// start using WinSock2.0 APIs
	pcModule = "WS2_32.DLL";
	m_hLib = LoadLibrary( pcModule );
	if (!m_hLib)
	{
	    // fallback to WinSock1.1
	    pcModule = "WSOCK32.DLL";
	    m_hLib = LoadLibrary( pcModule );
	}
#elif defined( _WINDOWS )
	pcModule = "AOLSOCK.AOL";
	if (GetModuleHandle( pcModule ))
	{
	    m_hLib = LoadLibrary( pcModule );
	}
	else
	{
		pcModule = "WINSOCK.DLL";
		m_hLib = LoadLibrary( pcModule );
	}
#else
#	error No Socket library to bind to.
#endif
	
#ifndef WIN32_PLATFORM_PSPC
	if (m_hLib > (HINSTANCE)HINSTANCE_ERROR)
#else
	if (m_hLib)
#endif
	{
		_hxaccept = (ACCEPT)HXGetProcAddress( m_hLib, "accept" );
		_hxbind = (BIND)HXGetProcAddress( m_hLib, "bind" );
		_hxclosesocket = (CLOSESOCKET)HXGetProcAddress( m_hLib, "closesocket" );
		_hxconnect = (CONNECT)HXGetProcAddress( m_hLib, "connect" );
		_hxioctlsocket = (IOCTLSOCKET)HXGetProcAddress( m_hLib, "ioctlsocket" );
		_hxgetpeername = (GETPEERNAME)HXGetProcAddress( m_hLib, "getpeername" );
		_hxgetsockname = (GETSOCKNAME)HXGetProcAddress( m_hLib, "getsockname" );
		_hxgetsockopt = (GETSOCKOPT)HXGetProcAddress( m_hLib, "getsockopt" );
		_hxhtonl = (HTONL)HXGetProcAddress( m_hLib, "htonl" );
		_hxhtons = (HTONS)HXGetProcAddress( m_hLib, "htons" );
		_hxinet_addr = (INET_ADDR)HXGetProcAddress( m_hLib, "inet_addr" );
		_hxinet_ntoa = (INET_NTOA)HXGetProcAddress( m_hLib, "inet_ntoa" );
		_hxlisten = (LISTEN)HXGetProcAddress( m_hLib, "listen" );
		_hxntohl = (NTOHL)HXGetProcAddress( m_hLib, "ntohl" );
		_hxntohs = (NTOHS)HXGetProcAddress( m_hLib, "ntohs" );
		_hxrecv = (RECV)HXGetProcAddress( m_hLib, "recv" );
		_hxrecvfrom = (RECVFROM)HXGetProcAddress( m_hLib, "recvfrom" );
		_hxselect = (SELECT)HXGetProcAddress( m_hLib, "select" );
		_hxsend = (SEND)HXGetProcAddress( m_hLib, "send" );
		_hxsendto = (SENDTO)HXGetProcAddress( m_hLib, "sendto" );
		_hxsetsockopt = (SETSOCKOPT)HXGetProcAddress( m_hLib, "setsockopt" );
		_hxshutdown = (SHUTDOWN)HXGetProcAddress( m_hLib, "shutdown" );
		_hxsocket = (HXSOCKET)HXGetProcAddress( m_hLib, "socket" );
		_hxgethostbyaddr = (GETHOSTBYADDR)HXGetProcAddress( m_hLib, "gethostbyaddr" );
		_hxgethostbyname = (GETHOSTBYNAME)HXGetProcAddress( m_hLib, "gethostbyname" );
		_hxgethostname = (GETHOSTNAME)HXGetProcAddress( m_hLib, "gethostname" );
#ifndef _WINCE
		_hxgetservbyport = (GETSERVBYPORT)HXGetProcAddress( m_hLib, "getservbyport" );
		_hxgetservbyname = (GETSERVBYNAME)HXGetProcAddress( m_hLib, "getservbyname" );
		_hxgetprotobynumber = (GETPROTOBYNUMBER)HXGetProcAddress( m_hLib, "getprotobynumber" );
		_hxgetprotobyname = (GETPROTOBYNAME)HXGetProcAddress( m_hLib, "getprotobyname" );
#endif /* _WINCE */
		_hxWSAStartup = (WSASTARTUP)HXGetProcAddress( m_hLib, "WSAStartup" );
		_hxWSACleanup = (WSACLEANUP)HXGetProcAddress( m_hLib, "WSACleanup" );
#ifdef _WINCE
		_hxWSASetLastError = (WSASETLASTERROR)SetLastError;
		_hxWSAGetLastError = (WSAGETLASTERROR)GetLastError;
#else
		_hxWSASetLastError = (WSASETLASTERROR)HXGetProcAddress( m_hLib, "WSASetLastError" );
		_hxWSAGetLastError = (WSAGETLASTERROR)HXGetProcAddress( m_hLib, "WSAGetLastError" );
		_hxWSAIsBlocking = (WSAISBLOCKING)HXGetProcAddress( m_hLib, "WSAIsBlocking" );
		_hxWSAUnhookBlockingHook = (WSAUNHOOKBLOCKINGHOOK)HXGetProcAddress( m_hLib, "WSAUnhookBlockingHook" );
		_hxWSASetBlockingHook = (WSASETBLOCKINGHOOK)HXGetProcAddress( m_hLib, "WSASetBlockingHook" );
		_hxWSACancelBlockingCall = (WSACANCELBLOCKINGCALL)HXGetProcAddress( m_hLib, "WSACancelBlockingCall" );
		_hxWSAAsyncGetServByName = (WSAASYNCGETSERVBYNAME)HXGetProcAddress( m_hLib, "WSAAsyncGetServByName" );
		_hxWSAAsyncGetServByPort = (WSAASYNCGETSERVBYPORT)HXGetProcAddress( m_hLib, "WSAAsyncGetServByPort" );
		_hxWSAAsyncGetProtoByName = (WSAASYNCGETPROTOBYNAME)HXGetProcAddress( m_hLib, "WSAAsyncGetProtoByName" );
		_hxWSAAsyncGetProtoByNumber = (WSAASYNCGETPROTOBYNUMBER)HXGetProcAddress( m_hLib, "WSAAsyncGetProtoByNumber" );
		_hxWSAAsyncGetHostByName = (WSAASYNCGETHOSTBYNAME)HXGetProcAddress( m_hLib, "WSAAsyncGetHostByName" );
		_hxWSAAsyncGetHostByAddr = (WSAASYNCGETHOSTBYADDR)HXGetProcAddress( m_hLib, "WSAAsyncGetHostByAddr" );
		_hxWSACancelAsyncRequest = (WSACANCELASYNCREQUEST)HXGetProcAddress( m_hLib, "WSACancelAsyncRequest" );
		_hxWSAAsyncSelect = (WSAASYNCSELECT)HXGetProcAddress( m_hLib, "WSAAsyncSelect" );
#endif /* _WINCE */
		_hxWSAFDIsSet = (__WSAFDISSET)HXGetProcAddress( m_hLib, "__WSAFDIsSet" );
		
		SetInited();
	}
	else
	{
		m_hLib = NULL;
		//	Init failed on creation, set all our function pointers to NULL
		_hxaccept =  NULL;
		_hxbind =  NULL;
		_hxclosesocket =  NULL;
		_hxconnect =  NULL;
		_hxioctlsocket = NULL;
		_hxgetpeername = NULL;
		_hxgetsockname = NULL;
		_hxgetsockopt = NULL;
		_hxhtonl = NULL;
		_hxhtons = NULL;
		_hxinet_addr = NULL;
		_hxinet_ntoa = NULL;
		_hxlisten = NULL;
		_hxntohl = NULL;
		_hxntohs = NULL;
		_hxrecv = NULL;
		_hxrecvfrom = NULL;
		_hxselect = NULL;
		_hxsend = NULL;
		_hxsendto = NULL;
		_hxsetsockopt = NULL;
		_hxshutdown = NULL;
		_hxsocket = NULL;
		_hxgethostbyaddr = NULL;
		_hxgethostbyname = NULL;
		_hxgethostname = NULL;
#ifndef _WINCE
		_hxgetservbyport = NULL;
		_hxgetservbyname = NULL;
		_hxgetprotobynumber = NULL;
		_hxgetprotobyname = NULL;
#endif /* _WINCE */
		_hxWSAStartup = NULL;
		_hxWSACleanup = NULL;
		_hxWSASetLastError = NULL;
		_hxWSAGetLastError = NULL;
#ifndef _WINCE
		_hxWSAIsBlocking = NULL;
		_hxWSAUnhookBlockingHook = NULL;
		_hxWSASetBlockingHook = NULL;
		_hxWSACancelBlockingCall = NULL;
		_hxWSAAsyncGetServByName = NULL;
		_hxWSAAsyncGetServByPort = NULL;
		_hxWSAAsyncGetProtoByName = NULL;
		_hxWSAAsyncGetProtoByNumber = NULL;
		_hxWSAAsyncGetHostByName = NULL;
		_hxWSAAsyncGetHostByAddr = NULL;
		_hxWSACancelAsyncRequest = NULL;
		_hxWSAAsyncSelect = NULL;
#endif /* _WINCE */
		_hxWSAFDIsSet = NULL;
	
		m_iInited = FALSE;
	}
	return( WinSockAvail() );
}

void CHXSock::UnloadWinsock(HXBOOL bDestuction)
{
	if (m_hLib)
	{
		FreeLibrary( m_hLib );
	}

	m_hLib = (HINSTANCE)0;
	m_iInited = FALSE;
}


void CHXSock::SetInited( void )
{
	if (!m_hLib)
	{
		return;
	}
	
	if (!_hxaccept || !_hxbind || !_hxclosesocket || !_hxconnect || 
	 !_hxioctlsocket || !_hxgetpeername || !_hxgetsockname || !_hxgetsockopt ||
	 !_hxhtonl || !_hxhtons || !_hxinet_addr || !_hxinet_ntoa || !_hxlisten ||
	 !_hxntohl || !_hxntohs || !_hxrecv || !_hxrecvfrom || !_hxselect || 
	 !_hxsend || !_hxsendto || !_hxsetsockopt || !_hxshutdown || !_hxsocket ||
	 !_hxgethostbyaddr || !_hxgethostbyname || !_hxgethostname || 
	 !_hxWSAStartup || !_hxWSACleanup || 
	 !_hxWSASetLastError || !_hxWSAGetLastError || !_hxWSAFDIsSet
#ifndef _WINCE
	 || !_hxgetservbyport || !_hxgetservbyname || !_hxgetprotobynumber ||
	 !_hxgetprotobyname || !_hxWSAUnhookBlockingHook || !_hxWSAIsBlocking ||
	 !_hxWSAAsyncGetHostByAddr || !_hxWSASetBlockingHook || 
	 !_hxWSACancelBlockingCall || !_hxWSAAsyncGetServByName || 
	 !_hxWSAAsyncGetServByPort || !_hxWSAAsyncGetProtoByName || 
	 !_hxWSAAsyncSelect || !_hxWSAAsyncGetProtoByNumber || 
	 !_hxWSACancelAsyncRequest || !_hxWSAAsyncGetHostByName 
#endif /* _WINCE */
	 )
	{
		m_iInited = FALSE;
		HX_ASSERT( m_iInited );
		if (m_hLib)
		{
			FreeLibrary( m_hLib );
			m_hLib = NULL;
		}
	}
	else
	{
		m_iInited = TRUE;
	}
}

HXBOOL CHXSock::InitWinsock( void )
{
	HXBOOL bInited = FALSE;

	int			err;                
	WORD		wVersionRequested;
	WSADATA		wsaData;
	
	wVersionRequested = (1 << 8) + 1;
	
	err = HXWSAStartup( wVersionRequested, &wsaData );

	// Check Winsock is the correct version
	if (err == 0) 
	{
	    if (LOBYTE( wsaData.wVersion ) == 1 && HIBYTE( wsaData.wVersion ) == 1) 
	    {
		bInited = TRUE;
	    }
	    else
	    {
		HXWSACleanup();
	    }
	}

	m_nVersion = LOBYTE(wsaData.wHighVersion);

	return bInited;
}

UINT8 CHXSock::HXGetVersion(void)
{
    return m_nVersion;
}

