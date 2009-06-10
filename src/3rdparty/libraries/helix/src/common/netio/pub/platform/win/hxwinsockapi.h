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
 * Synopsis:
 *
 * Declarations and typedefs for WinSock api function.
 * Declaration of HXWinsockAPI for storing WinSock function pointers.
 *
 *
 * ***** END LICENSE BLOCK ***** */ 

#if !defined( HX_WINSOCKAPI_H__ )
#define	HX_WINSOCKAPI_H__

#include "nettypes.h"

//
// WinSock 1.1
//
typedef SOCKET (PASCAL FAR* PFN_ACCEPT)( SOCKET, struct sockaddr FAR *, int FAR * );
typedef int (PASCAL FAR* PFN_BIND)( SOCKET, const struct sockaddr FAR *, int );
typedef int (PASCAL FAR* PFN_CLOSESOCKET)( SOCKET );
typedef int (PASCAL FAR* PFN_CONNECT)( SOCKET, const struct sockaddr FAR *, int );
typedef int (PASCAL FAR* PFN_IOCTLSOCKET)( SOCKET, long, u_long FAR * );
typedef int (PASCAL FAR* PFN_GETPEERNAME)( SOCKET, struct sockaddr FAR *, int FAR * );
typedef int (PASCAL FAR* PFN_GETSOCKNAME)( SOCKET, struct sockaddr FAR *, int FAR * );
typedef int (PASCAL FAR* PFN_GETSOCKOPT)( SOCKET, int, int, char FAR *, int FAR * );
typedef u_long (PASCAL FAR* PFN_HTONL)( u_long );
typedef u_short (PASCAL FAR* PFN_HTONS)( u_short );
typedef unsigned long (PASCAL FAR* PFN_INET_ADDR)( const char FAR * );
typedef char FAR * (PASCAL FAR* PFN_INET_NTOA)( struct in_addr );
typedef int (PASCAL FAR* PFN_LISTEN)( SOCKET, int );
typedef u_long (PASCAL FAR* PFN_NTOHL)( u_long );
typedef u_short (PASCAL FAR* PFN_NTOHS)( u_short );
typedef int (PASCAL FAR* PFN_RECV)( SOCKET, char FAR *, int, int );
typedef int (PASCAL FAR* PFN_RECVFROM)( SOCKET, char FAR *, int, int, struct sockaddr FAR *, int FAR * );
typedef int (PASCAL FAR* PFN_SELECT)( int, fd_set FAR *, fd_set FAR *, fd_set FAR *, const struct timeval FAR * );
typedef int (PASCAL FAR* PFN_SEND)( SOCKET, const char FAR *, int, int );
typedef int (PASCAL FAR* PFN_SENDTO)( SOCKET, const char FAR *, int, int, const struct sockaddr FAR *, int );
typedef int (PASCAL FAR* PFN_SETSOCKOPT)( SOCKET, int, int, const char FAR *, int );
typedef int (PASCAL FAR* PFN_SHUTDOWN)( SOCKET, int );
typedef SOCKET (PASCAL FAR* PFN_SOCKET)( int, int, int );
typedef struct hostent FAR * (PASCAL FAR* PFN_GETHOSTBYADDR)( const char FAR *, int, int );
typedef struct hostent FAR * (PASCAL FAR* PFN_GETHOSTBYNAME)( const char FAR * );
typedef int (PASCAL FAR* PFN_GETHOSTNAME)( char FAR *, int );
typedef int (PASCAL FAR* PFN_WSASTARTUP)( WORD, LPWSADATA );
typedef int (PASCAL FAR* PFN_WSACLEANUP)( void );
typedef void (PASCAL FAR* PFN_WSASETLASTERROR)( int );
typedef int (PASCAL FAR* PFN_WSAGETLASTERROR)( void );
typedef int (PASCAL FAR* PFN___WSAFDISSET)( SOCKET, fd_set FAR * );


//
// IPv6 winsock 2.2
//
typedef int (WINAPI *PFN_GETADDRINFO) (const char*,const char*,const struct addrinfo*,struct addrinfo**);
typedef int (WINAPI *PFN_GETNAMEINFO) (const struct sockaddr*,socklen_t,char*,size_t,char*,size_t,int);
typedef void (WINAPI *PFN_FREEADDRINFO) (struct addrinfo*);

//
// NB: not available on wince
//
#if !defined(WINCE)
typedef struct servent FAR* (PASCAL FAR* PFN_GETSERVBYPORT)( int, const char FAR * );
typedef struct servent FAR* (PASCAL FAR* PFN_GETSERVBYNAME)( const char FAR *, const char FAR * );
typedef struct protoent FAR* (PASCAL FAR* PFN_GETPROTOBYNUMBER)( int );
typedef struct protoent FAR* (PASCAL FAR* PFN_GETPROTOBYNAME)( const char FAR * );

typedef HANDLE (PASCAL FAR* PFN_WSAASYNCGETSERVBYNAME)( HWND, u_int, const char FAR *, const char FAR *, char FAR *, int );
typedef HANDLE (PASCAL FAR* PFN_WSAASYNCGETSERVBYPORT)( HWND, u_int, int, const char FAR *, char FAR *, int );
typedef HANDLE (PASCAL FAR* PFN_WSAASYNCGETPROTOBYNAME)( HWND, u_int, const char FAR *, char FAR *, int );
typedef HANDLE (PASCAL FAR* PFN_WSAASYNCGETPROTOBYNUMBER)( HWND, u_int, int, char FAR *, int );
typedef HANDLE (PASCAL FAR* PFN_WSAASYNCGETHOSTBYNAME)( HWND, u_int, const char FAR *, char FAR *, int );
typedef HANDLE (PASCAL FAR* PFN_WSAASYNCGETHOSTBYADDR)( HWND, u_int, const char FAR *, int, int, const char FAR *, int );
typedef int (PASCAL FAR* PFN_WSACANCELASYNCREQUEST)( HANDLE );
typedef int (PASCAL FAR* PFN_WSAASYNCSELECT)( SOCKET, HWND, u_int, long );

typedef int (PASCAL FAR* PFN_WSASEND)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, 
                                   LPDWORD lpNumberOfBytesSent,DWORD dwFlags, 
                                   LPWSAOVERLAPPED lpOverlapped, 
                                   LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

typedef int (PASCAL FAR* PFN_WSASENDTO)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, 
                                   LPDWORD lpNumberOfBytesSent,DWORD dwFlags, 
                                   const struct sockaddr* lpTo, int iToLen, 
                                   LPWSAOVERLAPPED lpOverlapped, 
                                   LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

typedef int (PASCAL FAR* PFN_WSARECV)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                                    LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags,
                                    LPWSAOVERLAPPED lpOverlapped, 
                                    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

typedef int (PASCAL FAR* PFN_WSARECVFROM)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                                    LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, struct sockaddr* lpFrom,
                                    LPINT lpFromlen, LPWSAOVERLAPPED lpOverlapped, 
                                    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);


typedef int (PASCAL FAR* PFN_WSAIOCTL)(SOCKET s, DWORD dwIoControlCode, LPVOID lpvInBuffer,
                                       DWORD cbInBuffer, LPVOID lpvOutBuffer, DWORD cbOutBuffer,
                                       LPDWORD lpcbBytesReturned, LPWSAOVERLAPPED lpOverlapped,
                                       LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);


#if(0)
//
// NB: these are obsolete per 2.2 docs
//
typedef HXBOOL (PASCAL FAR* PFN_WSAISBLOCKING)( void );
typedef int (PASCAL FAR* PFN_WSAUNHOOKBLOCKINGHOOK)( void );
typedef FARPROC (PASCAL FAR* PFN_WSASETBLOCKINGHOOK)( FARPROC );
typedef int (PASCAL FAR* PFN_WSACANCELBLOCKINGCALL)( void );
#endif
#endif //!WINCE


struct HXWinsockAPI
{
    // BSD compatible
    PFN_ACCEPT			m_accept;
    PFN_BIND			m_bind;
    PFN_CLOSESOCKET		m_closesocket;
    PFN_CONNECT			m_connect;
    PFN_IOCTLSOCKET	        m_ioctlsocket;
    PFN_GETPEERNAME		m_getpeername;
    PFN_GETSOCKNAME		m_getsockname;
    PFN_GETSOCKOPT		m_getsockopt;
    PFN_HTONL			m_htonl;
    PFN_HTONS			m_htons;
    PFN_INET_ADDR		m_inet_addr;
    PFN_INET_NTOA		m_inet_ntoa;
    PFN_LISTEN			m_listen;
    PFN_NTOHL			m_ntohl;
    PFN_NTOHS			m_ntohs;
    PFN_RECV			m_recv;
    PFN_RECVFROM		m_recvfrom;
    PFN_SELECT			m_select;
    PFN_SEND			m_send;
    PFN_SENDTO			m_sendto;
    PFN_SETSOCKOPT		m_setsockopt;
    PFN_SHUTDOWN		m_shutdown;
    PFN_SOCKET		        m_socket;

    // Database
    PFN_GETHOSTBYADDR		m_gethostbyaddr;
    PFN_GETHOSTBYNAME		m_gethostbyname;
    PFN_GETHOSTNAME		m_gethostname;
#ifndef _WINCE
    PFN_GETSERVBYPORT		m_getservbyport;
    PFN_GETSERVBYNAME		m_getservbyname;
    PFN_GETPROTOBYNUMBER	m_getprotobynumber;
    PFN_GETPROTOBYNAME		m_getprotobyname;
#endif /* _WINCE */

    // Microsoft Windows Extension
    PFN_WSASTARTUP		    m_WSAStartup;
    PFN_WSACLEANUP		    m_WSACleanup;
    PFN_WSASETLASTERROR		    m_WSASetLastError;
    PFN_WSAGETLASTERROR		    m_WSAGetLastError;
#ifndef _WINCE
    //PFN_WSAISBLOCKING		    m_WSAIsBlocking;
    //PFN_WSAUNHOOKBLOCKINGHOOK     m_WSAUnhookBlockingHook;
    //PFN_WSASETBLOCKINGHOOK	    m_WSASetBlockingHook;
    //PFN_WSACANCELBLOCKINGCALL	    m_WSACancelBlockingCall;
    PFN_WSAASYNCGETSERVBYNAME	    m_WSAAsyncGetServByName;
    PFN_WSAASYNCGETSERVBYPORT	    m_WSAAsyncGetServByPort;
    PFN_WSAASYNCGETPROTOBYNAME	    m_WSAAsyncGetProtoByName;
    PFN_WSAASYNCGETPROTOBYNUMBER    m_WSAAsyncGetProtoByNumber;
    PFN_WSAASYNCGETHOSTBYNAME	    m_WSAAsyncGetHostByName;
    PFN_WSAASYNCGETHOSTBYADDR	    m_WSAAsyncGetHostByAddr;
    PFN_WSACANCELASYNCREQUEST	    m_WSACancelAsyncRequest;
    PFN_WSAASYNCSELECT		    m_WSAAsyncSelect;
    PFN_WSASEND                     m_WSASend;
    PFN_WSASENDTO                   m_WSASendTo;
    PFN_WSARECV                     m_WSARecv;
    PFN_WSARECVFROM                 m_WSARecvFrom;
    PFN_WSAIOCTL                    m_WSAIoctl;
#endif /* _WINCE */
    PFN___WSAFDISSET		    m___WSAFDIsSet; // 3 underscores

    // IPv6
    PFN_GETADDRINFO     m_getaddrinfo;
    PFN_GETNAMEINFO     m_getnameinfo;
    PFN_FREEADDRINFO    m_freeaddrinfo;
};


#endif //HX_WINSOCKAPI_H__



