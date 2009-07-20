/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsock.h,v 1.6 2005/03/14 19:36:37 bobclark Exp $
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

#if !defined( HXSOCK_H )
#define	HXSOCK_H

#include "hlxclib/sys/socket.h"

#include "hlxclib/assert.h"

//	Typedefs used to declare our function pointers, and for casting return value from
//	GetProcAddress().

typedef SOCKET (PASCAL FAR	*ACCEPT)( SOCKET, struct sockaddr FAR *, int FAR * );
typedef int (PASCAL FAR	*BIND)( SOCKET, const struct sockaddr FAR *, int );
typedef int (PASCAL FAR * CLOSESOCKET)( SOCKET );
typedef int (PASCAL FAR * CONNECT)( SOCKET, const struct sockaddr FAR *, int );
typedef int (PASCAL FAR	*IOCTLSOCKET)( SOCKET, long, u_long FAR * );
typedef int (PASCAL FAR *GETPEERNAME)( SOCKET, struct sockaddr FAR *, int FAR * );
typedef int (PASCAL FAR *GETSOCKNAME)( SOCKET, struct sockaddr FAR *, int FAR * );
typedef int (PASCAL FAR *GETSOCKOPT)( SOCKET, int, int, char FAR *, int FAR * );
typedef u_long (PASCAL FAR *HTONL)( u_long );
typedef u_short (PASCAL FAR	*HTONS)( u_short );
typedef unsigned long (PASCAL FAR *INET_ADDR)( const char FAR * );
typedef char FAR * (PASCAL FAR * INET_NTOA)( struct in_addr );
typedef int (PASCAL FAR *LISTEN)( SOCKET, int );
typedef u_long (PASCAL FAR * NTOHL)( u_long );
typedef u_short (PASCAL FAR * NTOHS)( u_short );
typedef int (PASCAL FAR * RECV)( SOCKET, char FAR *, int, int );
typedef int (PASCAL FAR * RECVFROM)( SOCKET, char FAR *, int, int, struct sockaddr FAR *, int FAR * );
typedef int (PASCAL FAR * SELECT)( int, fd_set FAR *, fd_set FAR *, fd_set FAR *, const struct timeval FAR * );
typedef int (PASCAL FAR * SEND)( SOCKET, const char FAR *, int, int );
typedef int (PASCAL FAR *SENDTO)( SOCKET, const char FAR *, int, int, const struct sockaddr FAR *, int );
typedef int (PASCAL FAR *SETSOCKOPT)( SOCKET, int, int, const char FAR *, int );
typedef int (PASCAL FAR *SHUTDOWN)( SOCKET, int );
typedef SOCKET (PASCAL FAR *HXSOCKET)( int, int, int );
typedef struct hostent FAR * (PASCAL FAR *GETHOSTBYADDR)( const char FAR *, int, int );
typedef struct hostent FAR * (PASCAL FAR *GETHOSTBYNAME)( const char FAR * );
typedef int (PASCAL FAR *GETHOSTNAME)( char FAR *, int );
#ifndef _WINCE
typedef struct servent FAR * (PASCAL FAR *GETSERVBYPORT)( int, const char FAR * );
typedef struct servent FAR * (PASCAL FAR *GETSERVBYNAME)( const char FAR *, const char FAR * );
typedef struct protoent FAR * (PASCAL FAR *GETPROTOBYNUMBER)( int );
typedef struct protoent FAR * (PASCAL FAR *GETPROTOBYNAME)( const char FAR * );
#endif /* _WINCE */
typedef int (PASCAL FAR	*WSASTARTUP)( WORD, LPWSADATA );
typedef int (PASCAL FAR *WSACLEANUP)( void );
typedef void (PASCAL FAR *WSASETLASTERROR)( int );
typedef int (PASCAL FAR *WSAGETLASTERROR)( void );
#ifndef _WINCE
typedef HXBOOL (PASCAL FAR *WSAISBLOCKING)( void );
typedef int (PASCAL FAR *WSAUNHOOKBLOCKINGHOOK)( void );
typedef FARPROC (PASCAL FAR *WSASETBLOCKINGHOOK)( FARPROC );
typedef int (PASCAL FAR *WSACANCELBLOCKINGCALL)( void );
typedef HANDLE (PASCAL FAR *WSAASYNCGETSERVBYNAME)( HWND, u_int, const char FAR *, const char FAR *, char FAR *, int );
typedef HANDLE (PASCAL FAR *WSAASYNCGETSERVBYPORT)( HWND, u_int, int, const char FAR *, char FAR *, int );
typedef HANDLE (PASCAL FAR *WSAASYNCGETPROTOBYNAME)( HWND, u_int, const char FAR *, char FAR *, int );
typedef HANDLE (PASCAL FAR *WSAASYNCGETPROTOBYNUMBER)( HWND, u_int, int, char FAR *, int );
typedef HANDLE (PASCAL FAR *WSAASYNCGETHOSTBYNAME)( HWND, u_int, const char FAR *, char FAR *, int );
typedef HANDLE (PASCAL FAR *WSAASYNCGETHOSTBYADDR)( HWND, u_int, const char FAR *, int, int, const char FAR *, int );
typedef int (PASCAL FAR *WSACANCELASYNCREQUEST)( HANDLE );
typedef int (PASCAL FAR *WSAASYNCSELECT)( SOCKET, HWND, u_int, long );
#endif /* _WINCE */
typedef int (PASCAL FAR *__WSAFDISSET)( SOCKET, fd_set FAR * );

class CHXSock
{                                  
public:
	CHXSock();
	~CHXSock();
	
	HXBOOL InitWinsock();
	HXBOOL WinSockAvail()		{ return( m_iInited ); }
	
   /*
   	*	These are the exported methods for this class, basically it's a wrapper for winsock
    */

	//	This function branches on whether the socket is TCP or UDP 
	//	(different code for getting through a Firewall)
	int PASCAL FAR		HXconnect( SOCKET s, const struct sockaddr FAR *name, int namelen );
	
	//	This function branches on whether the socket is TCP or UDP 
	//	(different code for getting through a Firewall)
	int PASCAL FAR		HXbind ( SOCKET s, const struct sockaddr FAR *addr, int namelen );
	
	//	This function branches on whether the socket is TCP or UDP 
	//	(different code for getting through a Firewall)
	int PASCAL FAR		HXlisten( SOCKET s, int backlog );
	
	//	This function branches on whether the socket is TCP or UDP 
	//	(different code for getting through a Firewall)
	int PASCAL FAR		HXgetsockname( SOCKET s, struct sockaddr FAR *name, int FAR * namelen );

	//	This function branches on whether the socket is TCP or UDP 
	//	(different code for getting through a Firewall)
	u_int PASCAL FAR	HXaccept( SOCKET s, struct sockaddr FAR *addr, int FAR *addrlen );

	int PASCAL FAR		HXclosesocket( SOCKET s );
	
	int PASCAL FAR		HXioctlsocket( SOCKET s, long cmd, u_long FAR *argp );
	
	int PASCAL FAR		HXgetpeername( SOCKET s, struct sockaddr FAR *name, int FAR * namelen );
	
	int PASCAL FAR		HXgetsockopt( SOCKET s, int level, int optname, char FAR * optval, int FAR *optlen );
	
	u_long PASCAL FAR	HXhtonl( u_long hostlong );
	
	u_short PASCAL FAR	HXhtons( u_short hostshort );
	
	unsigned long PASCAL FAR HXinet_addr( const char FAR * cp );
	
	char FAR * PASCAL FAR	HXinet_ntoa( struct in_addr in );
	
	u_long PASCAL FAR	HXntohl( u_long netlong );
	
	u_short PASCAL FAR	HXntohs( u_short netshort );
	
	int PASCAL FAR		HXrecv( SOCKET s, char FAR * buf, int len, int flags );
	
	int PASCAL FAR		HXrecvfrom( SOCKET s, char FAR * buf, int len, int flags, struct sockaddr FAR *from, int FAR * fromlen );
	
	int PASCAL FAR		HXselect( int nfds, fd_set FAR *readfds, fd_set FAR *writefds, fd_set FAR *exceptfds, const struct timeval FAR *timeout );

	int PASCAL FAR		HXsend( SOCKET s, const char FAR * buf, int len, int flags );
	
	int PASCAL FAR		HXsendto( SOCKET s, const char FAR * buf, int len, int flags, const struct sockaddr FAR *to, int tolen );
	
	int PASCAL FAR		HXsetsockopt( SOCKET s, int level, int optname, const char FAR * optval, int optlen );
	
	int PASCAL FAR		HXshutdown( SOCKET s, int how );
	
	SOCKET PASCAL FAR	HXsocket( int af, int type, int protocol );
	
	/* Database function prototypes */
	struct hostent FAR * PASCAL FAR		HXgethostbyaddr( const char FAR * addr, int len, int type );
	
	struct hostent FAR * PASCAL FAR		HXgethostbyname( const char FAR * name );
	
	int PASCAL FAR 						HXgethostname( char FAR * name, int namelen );
	
#ifndef _WINCE
	struct servent FAR * PASCAL FAR		HXgetservbyport( int port, const char FAR * proto );
	
	struct servent FAR * PASCAL FAR		HXgetservbyname( const char FAR * name,
										 const char FAR * proto );
	
	struct protoent FAR * PASCAL FAR	HXgetprotobynumber( int proto );
	
	struct protoent FAR * PASCAL FAR	HXgetprotobyname( const char FAR * name );
#endif /* _WINCE */

	/* Microsoft Windows Extension function prototypes */
	int PASCAL FAR		HXWSAStartup( WORD wVersionRequired, LPWSADATA lpWSAData );
	
	int PASCAL FAR		HXWSACleanup( void );
	
	void PASCAL FAR		HXWSASetLastError( int iError );
	
	int PASCAL FAR		HXWSAGetLastError( void );
	
#ifndef _WINCE
	HXBOOL PASCAL FAR		HXWSAIsBlocking( void );
	
	int PASCAL FAR		HXWSAUnhookBlockingHook( void );
	
	FARPROC PASCAL FAR	HXWSASetBlockingHook( FARPROC lpBlockFunc );
	
	int PASCAL FAR		HXWSACancelBlockingCall( void );
	
	HANDLE PASCAL FAR	HXWSAAsyncGetServByName( HWND hWnd, u_int wMsg, const char FAR * name, const char FAR * proto, char FAR * buf, int buflen );
	
	HANDLE PASCAL FAR	HXWSAAsyncGetServByPort( HWND hWnd, u_int wMsg, int port, const char FAR * proto, char FAR * buf, int buflen );
	
	HANDLE PASCAL FAR	HXWSAAsyncGetProtoByName( HWND hWnd, u_int wMsg, const char FAR * name, char FAR * buf, int buflen );
	
	HANDLE PASCAL FAR	HXWSAAsyncGetProtoByNumber( HWND hWnd, u_int wMsg, int number, char FAR * buf, int buflen );
	
	HANDLE PASCAL FAR	HXWSAAsyncGetHostByName( HWND hWnd, u_int wMsg, const char FAR * name, char FAR * buf, int buflen );
	
	HANDLE PASCAL FAR	HXWSAAsyncGetHostByAddr( HWND hWnd, u_int wMsg, const char FAR * addr, int len, int type, const char FAR * buf, int buflen );
	
	int PASCAL FAR		HXWSACancelAsyncRequest( HANDLE hAsyncTaskHandle );

	int PASCAL FAR HXWSAAsyncSelect( SOCKET s, HWND hWnd, u_int wMsg, long lEvent);
#endif /* _WINCE */
	
	int PASCAL FAR HX__WSAFDIsSet( SOCKET s, fd_set FAR *fdSet );

	UINT8			HXGetVersion(void);

protected:
	int			m_iInited;
	HINSTANCE		m_hLib;
	UINT8			m_nVersion;
	
	void SetInited( void );
	
	//	Internal function pointers
	ACCEPT						_hxaccept;
	BIND						_hxbind;
	CLOSESOCKET					_hxclosesocket;
	CONNECT						_hxconnect;
	IOCTLSOCKET					_hxioctlsocket;
	GETPEERNAME					_hxgetpeername;
	GETSOCKNAME					_hxgetsockname;
	GETSOCKOPT					_hxgetsockopt;
	HTONL						_hxhtonl;
	HTONS						_hxhtons;
	INET_ADDR					_hxinet_addr;
	INET_NTOA					_hxinet_ntoa;
	LISTEN						_hxlisten;
	NTOHL						_hxntohl;
	NTOHS						_hxntohs;
	RECV						_hxrecv;
	RECVFROM					_hxrecvfrom;
	SELECT						_hxselect;
	SEND						_hxsend;
	SENDTO						_hxsendto;
	SETSOCKOPT					_hxsetsockopt;
	SHUTDOWN					_hxshutdown;
	HXSOCKET					_hxsocket;
	
	/* Database function prototypes */
	GETHOSTBYADDR				_hxgethostbyaddr;
	GETHOSTBYNAME				_hxgethostbyname;
	GETHOSTNAME					_hxgethostname;
#ifndef _WINCE
	GETSERVBYPORT				_hxgetservbyport;
	GETSERVBYNAME				_hxgetservbyname;
	GETPROTOBYNUMBER			_hxgetprotobynumber;
	GETPROTOBYNAME				_hxgetprotobyname;
#endif /* _WINCE */

	/* Microsoft Windows Extension function prototypes */
	WSASTARTUP					_hxWSAStartup;
	WSACLEANUP					_hxWSACleanup;
	WSASETLASTERROR				_hxWSASetLastError;
	WSAGETLASTERROR				_hxWSAGetLastError;
#ifndef _WINCE
	WSAISBLOCKING				_hxWSAIsBlocking;
	WSAUNHOOKBLOCKINGHOOK		_hxWSAUnhookBlockingHook;
	WSASETBLOCKINGHOOK			_hxWSASetBlockingHook;
	WSACANCELBLOCKINGCALL		_hxWSACancelBlockingCall;
	WSAASYNCGETSERVBYNAME		_hxWSAAsyncGetServByName;
	WSAASYNCGETSERVBYPORT		_hxWSAAsyncGetServByPort;
	WSAASYNCGETPROTOBYNAME		_hxWSAAsyncGetProtoByName;
	WSAASYNCGETPROTOBYNUMBER	_hxWSAAsyncGetProtoByNumber;
	WSAASYNCGETHOSTBYNAME		_hxWSAAsyncGetHostByName;
	WSAASYNCGETHOSTBYADDR		_hxWSAAsyncGetHostByAddr;
	WSACANCELASYNCREQUEST		_hxWSACancelAsyncRequest;
	WSAASYNCSELECT				_hxWSAAsyncSelect;
#endif /* _WINCE */
	__WSAFDISSET				_hxWSAFDIsSet;

	HXBOOL LoadWinsock( void );
	void UnloadWinsock(HXBOOL bDestuction = TRUE);
};

// This is the one CHXSock object that we create
extern CHXSock*		sockObj;

#define _INLINE_CHXSOCK_
//#include "sock.inl"
//#include "sock.h"

#define __WSAFDIsSet( arg1, arg2 )		sockObj->HX__WSAFDIsSet( arg1, arg2 )

#endif  
