/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sock.h,v 1.6 2007/07/06 20:44:00 jfinnecy Exp $
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

#if !defined( _SOCK_INL_ )
#define _SOCK_INL_

#include "hxsock.h"

/*
 *	First the CHXSock inlines
 */
//	This function branches on whether the socket is TCP or UDP 
//	(different code for getting through a Firewall)
#if defined( _INLINE_CHXSOCK_ )
__inline int PASCAL FAR		CHXSock::HXconnect( SOCKET s, const struct sockaddr FAR *name, int namelen )
{
	assert( m_hLib && _hxconnect /* && gpProxy */ );
	return( _hxconnect( s, (const struct ::sockaddr FAR *)name, namelen ) );
}

//	This function branches on whether the socket is TCP or UDP 
//	(different code for getting through a Firewall)
__inline int PASCAL FAR		 CHXSock::HXbind ( SOCKET s, const struct sockaddr FAR *addr, int namelen )
{
	assert( m_hLib && _hxbind /* && gpProxy */ );
	return( _hxbind( s, (const struct ::sockaddr FAR *)addr, namelen ) );
}

//	This function branches on whether the socket is TCP or UDP 
//	(different code for getting through a Firewall)
__inline int PASCAL FAR		CHXSock::HXlisten( SOCKET s, int backlog )
{
	assert( m_hLib && _hxlisten /* && gpProxy */ );
	return( _hxlisten( s, backlog ) );
}

//	This function branches on whether the socket is TCP or UDP 
//	(different code for getting through a Firewall)
__inline int PASCAL FAR		CHXSock::HXgetsockname( SOCKET s, struct sockaddr FAR *name, int FAR * namelen )
{
	assert( m_hLib && _hxgetsockname /* && gpProxy */ );
	return( _hxgetsockname( s, (struct ::sockaddr FAR *)name, namelen ) );
}

//	This function branches on whether the socket is TCP or UDP 
//	(different code for getting through a Firewall)
__inline u_int PASCAL FAR	CHXSock::HXaccept( SOCKET s, struct sockaddr FAR *addr, int FAR *addrlen )
{
	assert( m_hLib && _hxaccept /* && gpProxy */ );
	return( _hxaccept( s, (struct ::sockaddr FAR *)addr, addrlen ) );
}

__inline int PASCAL FAR		CHXSock::HXclosesocket( SOCKET s )
{
	assert( m_hLib && _hxclosesocket );
	return( _hxclosesocket( s ) );
}

__inline int PASCAL FAR		CHXSock::HXioctlsocket( SOCKET s, long cmd, u_long FAR *argp )
{
	assert( m_hLib && _hxioctlsocket );
	return( _hxioctlsocket( s, cmd, argp ) );
}

__inline int PASCAL FAR		CHXSock::HXgetpeername( SOCKET s, struct sockaddr FAR *name, int FAR * namelen )
{
	assert( m_hLib && _hxgetpeername );
	return( _hxgetpeername( s, name, namelen ) );
}

__inline int PASCAL FAR		CHXSock::HXgetsockopt( SOCKET s, int level, int optname, char FAR * optval, int FAR *optlen )
{
	assert( m_hLib && _hxgetsockopt );
	return( _hxgetsockopt( s, level, optname, optval, optlen ) );
}

__inline u_long PASCAL FAR	CHXSock::HXhtonl( u_long hostlong )
{
	assert( m_hLib && _hxhtonl );
	return( _hxhtonl( hostlong ) );
}

__inline u_short PASCAL FAR	CHXSock::HXhtons( u_short hostshort )
{
	assert( m_hLib && _hxhtons );
	return( _hxhtons( hostshort ) );
}

__inline unsigned long PASCAL FAR CHXSock::HXinet_addr( const char FAR * cp )
{
	assert( m_hLib && _hxinet_addr );
	return( _hxinet_addr( cp ) );
}

__inline char FAR * PASCAL FAR	CHXSock::HXinet_ntoa( struct in_addr in )
{
	assert( m_hLib && _hxinet_ntoa );
	return( _hxinet_ntoa( in ) );
}

__inline u_long PASCAL FAR	CHXSock::HXntohl( u_long netlong )
{
	assert( m_hLib && _hxntohl );
	return( _hxntohl( netlong ) );
}

__inline u_short PASCAL FAR	CHXSock::HXntohs( u_short netshort )
{
	assert( m_hLib && _hxntohs );
	return( _hxntohs( netshort ) );
}

__inline int PASCAL FAR		CHXSock::HXrecv( SOCKET s, char FAR * buf, int len, int flags )
{
	assert( m_hLib && _hxrecv );
	return( _hxrecv( s, buf, len, flags ) );
}

__inline int PASCAL FAR		CHXSock::HXrecvfrom( SOCKET s, char FAR * buf, int len, int flags, struct sockaddr FAR *from, int FAR * fromlen )
{
	assert( m_hLib && _hxrecvfrom );
	return( _hxrecvfrom( s, buf, len, flags, from, fromlen ) );
}

__inline int PASCAL FAR		CHXSock::HXselect( int nfds, fd_set FAR *readfds, fd_set FAR *writefds, fd_set FAR *exceptfds, const struct timeval FAR *timeout )
{
	assert( m_hLib && _hxselect );
	return( _hxselect( nfds, readfds, writefds, exceptfds, timeout ) );
}

__inline int PASCAL FAR		CHXSock::HXsend( SOCKET s, const char FAR * buf, int len, int flags )
{
	assert( m_hLib && _hxsend );
	return( _hxsend( s, buf, len, flags ) );
}

__inline int PASCAL FAR		CHXSock::HXsendto( SOCKET s, const char FAR * buf, int len, int flags, const struct sockaddr FAR *to, int tolen )
{
	assert( m_hLib && _hxsendto );
	return( _hxsendto( s, buf, len, flags, to, tolen ) );
}

__inline int PASCAL FAR		CHXSock::HXsetsockopt( SOCKET s, int level, int optname, const char FAR * optval, int optlen )
{
	assert( m_hLib && _hxsetsockopt );
	return( _hxsetsockopt( s, level, optname, optval, optlen ) );
}

__inline int PASCAL FAR		CHXSock::HXshutdown( SOCKET s, int how )
{
	assert( m_hLib && _hxshutdown );
	return( _hxshutdown( s, how ) );
}

__inline SOCKET PASCAL FAR	CHXSock::HXsocket( int af, int type, int protocol )
{
	assert( m_hLib && _hxsocket );
	return( _hxsocket( af, type, protocol ) );
}

/* Database function prototypes */
__inline struct hostent FAR * PASCAL FAR		CHXSock::HXgethostbyaddr( const char FAR * addr, int len, int type )
{
	assert( m_hLib && _hxgethostbyaddr );
	return( _hxgethostbyaddr( addr, len, type ) );
}

__inline struct hostent FAR * PASCAL FAR		CHXSock::HXgethostbyname( const char FAR * name )
{
	struct hostent FAR *lpHost;

	assert( m_hLib && _hxgethostbyname );

	lpHost = _hxgethostbyname( name );
	return( lpHost );
}

__inline int PASCAL FAR 						CHXSock::HXgethostname( char FAR * name, int namelen )
{
	assert( m_hLib && _hxgethostname );
	return( _hxgethostname( name, namelen ) );
}

#ifndef _WINCE
__inline struct servent FAR * PASCAL FAR		CHXSock::HXgetservbyport( int port, const char FAR * proto )
{
	assert( m_hLib && _hxgetservbyport );
	return( _hxgetservbyport( port, proto ) );
}

__inline struct servent FAR * PASCAL FAR		CHXSock::HXgetservbyname( const char FAR * name,
									 const char FAR * proto )
{
	assert( m_hLib && _hxgetservbyname );
	return( _hxgetservbyname( name, proto ) );
}

__inline struct protoent FAR * PASCAL FAR	CHXSock::HXgetprotobynumber( int proto )
{
	assert( m_hLib && _hxgetprotobynumber );
	return( _hxgetprotobynumber( proto ) );
}

__inline struct protoent FAR * PASCAL FAR	CHXSock::HXgetprotobyname( const char FAR * name )
{
	assert( m_hLib && _hxgetprotobyname );
	return( _hxgetprotobyname( name ) );
}
#endif /* _WINCE */

/* Microsoft Windows Extension function prototypes */
__inline int PASCAL FAR		CHXSock::HXWSAStartup( WORD wVersionRequired, LPWSADATA lpWSAData )
{
	int		iRet;

	assert( m_hLib && _hxWSAStartup );
	iRet = _hxWSAStartup( wVersionRequired, lpWSAData );
	if (iRet)
	{
		return( iRet );
	}
	else
	{
		return( 0 );
	}
}

__inline int PASCAL FAR		CHXSock::HXWSACleanup( void )
{
	assert( m_hLib && _hxWSACleanup );
	return( _hxWSACleanup() );
}

__inline void PASCAL FAR		CHXSock::HXWSASetLastError( int iError )
{
	assert( m_hLib && _hxWSASetLastError );
	_hxWSASetLastError( iError );
}

__inline int PASCAL FAR		CHXSock::HXWSAGetLastError( void )
{
	assert( m_hLib && _hxWSAGetLastError );
	return( _hxWSAGetLastError() );
}

#ifndef _WINCE
__inline HXBOOL PASCAL FAR		CHXSock::HXWSAIsBlocking( void )
{
	assert( m_hLib && _hxWSAIsBlocking );
	return( _hxWSAIsBlocking() );
}

__inline int PASCAL FAR		CHXSock::HXWSAUnhookBlockingHook( void )
{
	assert( m_hLib && _hxWSAUnhookBlockingHook );
	return( _hxWSAUnhookBlockingHook() );
}

__inline FARPROC PASCAL FAR	CHXSock::HXWSASetBlockingHook( FARPROC lpBlockFunc )
{
	assert( m_hLib && _hxWSASetBlockingHook );
	return( _hxWSASetBlockingHook( lpBlockFunc ) );
}

__inline int PASCAL FAR		CHXSock::HXWSACancelBlockingCall( void )
{
	assert( m_hLib && _hxWSACancelBlockingCall );
	return( _hxWSACancelBlockingCall() );
}

__inline HANDLE PASCAL FAR	CHXSock::HXWSAAsyncGetServByName( HWND hWnd, u_int wMsg, const char FAR * name, const char FAR * proto, char FAR * buf, int buflen )
{
	assert( m_hLib && _hxWSAAsyncGetServByName );
	return( _hxWSAAsyncGetServByName( hWnd, wMsg, name, proto, buf, buflen ) );
}

__inline HANDLE PASCAL FAR	CHXSock::HXWSAAsyncGetServByPort( HWND hWnd, u_int wMsg, int port, const char FAR * proto, char FAR * buf, int buflen )
{
	assert( m_hLib && _hxWSAAsyncGetServByPort );
	return( _hxWSAAsyncGetServByPort( hWnd, wMsg, port, proto, buf, buflen ) );
}

__inline HANDLE PASCAL FAR	CHXSock::HXWSAAsyncGetProtoByName( HWND hWnd, u_int wMsg, const char FAR * name, char FAR * buf, int buflen )
{
	assert( m_hLib && _hxWSAAsyncGetProtoByName );
	return( _hxWSAAsyncGetProtoByName( hWnd, wMsg, name, buf, buflen ) );
}

__inline HANDLE PASCAL FAR	CHXSock::HXWSAAsyncGetProtoByNumber( HWND hWnd, u_int wMsg, int number, char FAR * buf, int buflen )
{
	assert( m_hLib && _hxWSAAsyncGetProtoByNumber );
	return( _hxWSAAsyncGetProtoByNumber( hWnd, wMsg, number, buf, buflen ) );
}

__inline HANDLE PASCAL FAR	CHXSock::HXWSAAsyncGetHostByName( HWND hWnd, u_int wMsg, const char FAR * name, char FAR * buf, int buflen )
{
	assert( m_hLib && _hxWSAAsyncGetHostByName );
	return( _hxWSAAsyncGetHostByName( hWnd, wMsg, name, buf, buflen ) );
}

__inline HANDLE PASCAL FAR	CHXSock::HXWSAAsyncGetHostByAddr( HWND hWnd, u_int wMsg, const char FAR * addr, int len, int type, const char FAR * buf, int buflen )
{
	assert( m_hLib && _hxWSAAsyncGetHostByAddr );
	return( _hxWSAAsyncGetHostByAddr( hWnd, wMsg, addr, len, type, buf, buflen ) );
}

__inline int PASCAL FAR		CHXSock::HXWSACancelAsyncRequest( HANDLE hAsyncTaskHandle )
{
	assert( m_hLib && _hxWSACancelAsyncRequest );
	return( _hxWSACancelAsyncRequest( hAsyncTaskHandle ) );
}

__inline int PASCAL FAR CHXSock::HXWSAAsyncSelect( SOCKET s, HWND hWnd, u_int wMsg, long lEvent)
{
	assert( m_hLib && _hxWSAAsyncSelect );
	return( _hxWSAAsyncSelect( s, hWnd, wMsg, lEvent ) );
}
#endif /* _WINCE */

__inline int PASCAL FAR CHXSock::HX__WSAFDIsSet( SOCKET s, fd_set FAR *fdSet )
{
	assert( m_hLib && _hxWSAFDIsSet );
	return( _hxWSAFDIsSet( s, fdSet ) );
}
#undef _INLINE_CHXSOCK_
#endif	//	#if defined( _INLINE_CHXSOCK_ )

#endif	//	if !defined( _SOCK_INL_ )

