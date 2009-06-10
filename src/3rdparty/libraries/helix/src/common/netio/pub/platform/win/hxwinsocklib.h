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

//
// Synopsis:
//
// WinSock API wrapper. Dynamically loads/unloads winsock dll.
//
//


#if !defined( HX_WINSOCKLIB_H__ )
#define HX_WINSOCKLIB_H__

#if !defined(_WIN32)
#error windows header file only
#endif

#if !defined(HELIX_CONFIG_SOCKLIB_EXPLICIT_LOAD)
// linker directive to include winsock for static link
#pragma comment (lib, "ws2_32")
inline HX_RESULT HXLoadWinSock(IUnknown* pContext)
{
    return HXR_OK;
}
inline void HXReleaseWinSock()
{
    // empty
}
#else

#include "hxwinsockapi.h"
#include "hxassert.h"
#include "hxengin.h"

// these are #defined in winsock2 headers
#undef getaddrinfo
#undef getnameinfo
#undef freeaddrinfo

class HXMutex;

class HXWinSockLib
{
public:
// singleton access only
    static HXWinSockLib& Lib(IUnknown* pContext = NULL);
    ~HXWinSockLib();

public:
// methods
    HX_RESULT Load();
    void UnLoad();
    bool IsLoaded() const;
    bool HasIPv6NameResolution() const;

public:
// api wrappers - call these
    int connect(SOCKET s, const struct sockaddr FAR *name, int namelen);
    int bind(SOCKET s, const struct sockaddr FAR *addr, int namelen);
    int listen(SOCKET s, int backlog);
    int getsockname(SOCKET s, struct sockaddr FAR *name, int FAR * namelen);
    int accept(SOCKET s, struct sockaddr FAR *addr, int FAR *addrlen);
    int closesocket(SOCKET s);
    int ioctlsocket(SOCKET s, long cmd, u_long FAR *argp);
    int getpeername(SOCKET s, struct sockaddr FAR *name, int FAR * namelen);
    int getsockopt(SOCKET s, int level, int optname, char FAR * optval, int FAR *optlen);
    u_long HXWinSockLib::htonl(u_long hostlong);
    u_short htons(u_short hostshort);
    unsigned long inet_addr(const char FAR * cp);
    char FAR * inet_ntoa(struct in_addr in);
    u_long ntohl(u_long netlong);
    u_short ntohs(u_short netshort);
    int recv(SOCKET s, char FAR * buf, int len, int flags);
    int recvfrom(SOCKET s, char FAR * buf, int len, int flags, struct sockaddr FAR *from, int FAR * fromlen);
    int select(int nfds, fd_set FAR *readfds, fd_set FAR *writefds, fd_set FAR *exceptfds, const struct timeval FAR *timeout);
    int send(SOCKET s, const char FAR * buf, int len, int flags);
    int sendto(SOCKET s, const char FAR * buf, int len, int flags, const struct sockaddr FAR *to, int tolen);
    int setsockopt(SOCKET s, int level, int optname, const char FAR * optval, int optlen);
    int shutdown(SOCKET s, int how);
    SOCKET socket(int af, int type, int protocol);
    struct hostent FAR* gethostbyaddr( const char FAR * addr, int len, int type);
    struct hostent FAR* gethostbyname(const char FAR * name);
    int gethostname(char FAR * name, int namelen);
#if !defined(_WINCE)
    struct servent FAR * getservbyport(int port, const char FAR * proto);
    struct servent FAR * getservbyname(const char FAR * name, const char FAR * proto);
    struct protoent FAR * getprotobynumber(int proto);
    struct protoent FAR * getprotobyname( const char FAR * name);
#endif

    int WSAStartup(WORD wVersionRequired, LPWSADATA lpWSAData);
    int WSACleanup();
    void WSASetLastError(int iError);
    int WSAGetLastError();

#if !defined(_WINCE)
    HANDLE WSAAsyncGetServByName(HWND hWnd,u_int wMsg,const char FAR * name, const char FAR * proto, char FAR * buf, int buflen);
    HANDLE WSAAsyncGetServByPort( HWND hWnd, u_int wMsg, int port, const char FAR * proto, char FAR * buf, int buflen);
    HANDLE WSAAsyncGetProtoByName(HWND hWnd,u_int wMsg,const char FAR * name, char FAR * buf, int buflen);
    HANDLE WSAAsyncGetProtoByNumber(HWND hWnd, u_int wMsg, int number, char FAR * buf, int buflen);
    HANDLE WSAAsyncGetHostByName(HWND hWnd, u_int wMsg, const char FAR * name, char FAR * buf, int buflen);
    HANDLE WSAAsyncGetHostByAddr(HWND hWnd, u_int wMsg, const char FAR * addr, int len, int type, const char FAR * buf, int buflen);
    int WSACancelAsyncRequest(HANDLE hAsyncTaskHandle);
    int WSAAsyncSelect(SOCKET s, HWND hWnd, u_int wMsg, long lEvent);
    int WSASend(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, 
        LPDWORD lpNumberOfBytesSent, DWORD dwFlags, 
        LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
    int WSASendTo(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, 
        LPDWORD lpNumberOfBytesSent, DWORD dwFlags, const struct sockaddr* lpTo, 
        int iToLen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
    int WSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd,
                    LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped,
                    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
    int WSARecvFrom(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd,
                    LPDWORD lpFlags, struct sockaddr* lpFrom, LPINT lpFromlen, LPWSAOVERLAPPED lpOverlapped,
                    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
    int WSAIoctl(SOCKET s, DWORD dwIoControlCode, LPVOID lpvInBuffer, DWORD cbInBuffer, 
                LPVOID lpvOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbBytesReturned, 
                LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
#endif

    int getaddrinfo(const char* nodename, const char* servname,const struct addrinfo* hints, struct addrinfo** res /*out*/);
    int getnameinfo (const struct sockaddr* sa,socklen_t salen, char* host /*out*/, size_t hostlen,char* serv /*out*/,size_t servlen, int flags);
    void freeaddrinfo(struct addrinfo* ai);

    inline int __WSAFDIsSet(SOCKET s, fd_set FAR *fdSet);

private:
// ctor 
    HXWinSockLib(IUnknown* pContext);

// disallow copy
    HXWinSockLib& operator=(const HXWinSockLib&);
    HXWinSockLib(const HXWinSockLib&);

// implementation
    void WinsockCleanup();
    HX_RESULT TryWinsockInit(BYTE major, BYTE minor);
    HX_RESULT WinsockInit();

private:
    WORD                m_wsHighVersion; // highest version supported by loaded dll
    WORD                m_wsVersion;     // version we requested
    HINSTANCE	        m_hLib;          // handle to loaded library
    HXWinsockAPI        m_api;           // loaded api function pointers
    UINT32              m_userCount;     // lib user ref count
    IHXMutex*           m_pLoadMutex;    // lib may be shared among threads
};

inline
bool
HXWinSockLib::IsLoaded() const
{
    return m_hLib != NULL;
}

inline
bool 
HXWinSockLib::HasIPv6NameResolution() const
{
    return m_api.m_getaddrinfo != 0; 
}


inline
int HXWinSockLib::connect(SOCKET s, const struct sockaddr FAR *name, int namelen)
{
    return m_api.m_connect(s, name, namelen);
}

inline
int HXWinSockLib::bind(SOCKET s, const struct sockaddr FAR *addr, int namelen)
{
    return m_api.m_bind(s, addr, namelen);
}

inline
int HXWinSockLib::listen(SOCKET s, int backlog)
{
    return m_api.m_listen(s, backlog);
}

inline
int HXWinSockLib::getsockname(SOCKET s, struct sockaddr FAR *name, int FAR * namelen)
{
    return m_api.m_getsockname(s, name, namelen);
}

inline
int HXWinSockLib::accept(SOCKET s, struct sockaddr FAR *addr, int FAR *addrlen)
{
    return m_api.m_accept(s, addr, addrlen);
}

inline
int HXWinSockLib::closesocket(SOCKET s)
{
    return m_api.m_closesocket(s);
}


inline
int HXWinSockLib::ioctlsocket(SOCKET s, long cmd, u_long FAR *argp)
{
    return m_api.m_ioctlsocket(s, cmd, argp);
}

inline 
int HXWinSockLib::getpeername(SOCKET s, struct sockaddr FAR *name, int FAR * namelen)
{
    return m_api.m_getpeername(s, name, namelen);
}

inline 
int HXWinSockLib::getsockopt(SOCKET s, int level, int optname, char FAR * optval, int FAR *optlen)
{
    return m_api.m_getsockopt(s, level, optname, optval, optlen);
}

inline 
u_long HXWinSockLib::htonl(u_long hostlong)
{
    return m_api.m_htonl(hostlong);
}

inline 
u_short HXWinSockLib::htons(u_short hostshort)
{
    return m_api.m_htons(hostshort);
}

inline 
unsigned long HXWinSockLib::inet_addr(const char FAR * cp)
{
    return m_api.m_inet_addr(cp);
}

inline 
char FAR * HXWinSockLib::inet_ntoa(struct in_addr in)
{
    return m_api.m_inet_ntoa(in);
}

inline 
u_long HXWinSockLib::ntohl(u_long netlong)
{
    return m_api.m_ntohl(netlong);
}

inline 
u_short HXWinSockLib::ntohs(u_short netshort)
{
    return m_api.m_ntohs(netshort);
}

inline 
int HXWinSockLib::recv(SOCKET s, char FAR * buf, int len, int flags)
{
    return m_api.m_recv(s, buf, len, flags);
}

inline 
int HXWinSockLib::recvfrom(SOCKET s, char FAR * buf, int len, int flags, struct sockaddr FAR *from, int FAR * fromlen)
{
    return m_api.m_recvfrom(s, buf, len, flags, from, fromlen);
}

inline 
int HXWinSockLib::select(int nfds, fd_set FAR *readfds, fd_set FAR *writefds, fd_set FAR *exceptfds, const struct timeval FAR *timeout)
{
    return m_api.m_select(nfds, readfds, writefds, exceptfds, timeout);
}

inline 
int HXWinSockLib::send(SOCKET s, const char FAR * buf, int len, int flags)
{
    return m_api.m_send(s, buf, len, flags);
}

inline 
int HXWinSockLib::sendto(SOCKET s, const char FAR * buf, int len, int flags, const struct sockaddr FAR *to, int tolen)
{
    return m_api.m_sendto(s, buf, len, flags, to, tolen);
}

inline
int HXWinSockLib::setsockopt(SOCKET s, int level, int optname, const char FAR * optval, int optlen)
{
    return m_api.m_setsockopt(s, level, optname, optval, optlen);
}

inline 
int HXWinSockLib::shutdown(SOCKET s, int how)
{
    return m_api.m_shutdown(s, how);
}

inline 
SOCKET HXWinSockLib::socket(int af, int type, int protocol)
{
    return m_api.m_socket(af, type, protocol);
}

inline 
struct hostent FAR* HXWinSockLib::gethostbyaddr( const char FAR * addr, int len, int type)
{
    return m_api.m_gethostbyaddr(addr, len, type);
}

inline 
struct hostent FAR* HXWinSockLib::gethostbyname(const char FAR * name)
{
    return  m_api.m_gethostbyname(name);
}

inline 
int HXWinSockLib::gethostname(char FAR * name, int namelen)
{
    return m_api.m_gethostname(name, namelen);
}

#ifndef _WINCE
inline 
struct servent FAR * HXWinSockLib::getservbyport(int port, const char FAR * proto)
{
    return m_api.m_getservbyport(port, proto);
}

inline 
struct servent FAR * HXWinSockLib::getservbyname(const char FAR * name, const char FAR * proto)
{
    return m_api.m_getservbyname(name, proto);
}

inline struct protoent FAR * HXWinSockLib::getprotobynumber(int proto)
{
    return m_api.m_getprotobynumber(proto);
}

inline struct protoent FAR * HXWinSockLib::getprotobyname( const char FAR * name)
{
    return m_api.m_getprotobyname(name);
}
#endif /* _WINCE */

inline 
int HXWinSockLib::WSAStartup(WORD wVersionRequired, LPWSADATA lpWSAData)
{
    return m_api.m_WSAStartup(wVersionRequired, lpWSAData);	
}
inline 
int HXWinSockLib::WSACleanup()
{
    return m_api.m_WSACleanup();
}

inline
void HXWinSockLib::WSASetLastError(int iError)
{
    m_api.m_WSASetLastError( iError );
}

inline 
int HXWinSockLib::WSAGetLastError()
{
    return m_api.m_WSAGetLastError();
}

#ifndef _WINCE


inline 
HANDLE HXWinSockLib::WSAAsyncGetServByName(HWND hWnd,u_int wMsg,const char FAR * name, const char FAR * proto, char FAR * buf, int buflen)
{
    return m_api.m_WSAAsyncGetServByName( hWnd, wMsg, name, proto, buf, buflen);
}

inline 
HANDLE HXWinSockLib::WSAAsyncGetServByPort( HWND hWnd, u_int wMsg, int port, const char FAR * proto, char FAR * buf, int buflen )
{
    return m_api.m_WSAAsyncGetServByPort(hWnd, wMsg, port, proto, buf, buflen);
}

inline 
HANDLE HXWinSockLib::WSAAsyncGetProtoByName(HWND hWnd,u_int wMsg,const char FAR * name, char FAR * buf, int buflen)
{
    return m_api.m_WSAAsyncGetProtoByName( hWnd, wMsg, name, buf, buflen);
}

inline 
HANDLE HXWinSockLib::WSAAsyncGetProtoByNumber(HWND hWnd, u_int wMsg, int number, char FAR * buf, int buflen )
{
    return m_api.m_WSAAsyncGetProtoByNumber( hWnd, wMsg, number, buf, buflen);
}

inline HANDLE HXWinSockLib::WSAAsyncGetHostByName(HWND hWnd, u_int wMsg, const char FAR * name, char FAR * buf, int buflen)
{
    return m_api.m_WSAAsyncGetHostByName( hWnd, wMsg, name, buf, buflen);
}

inline HANDLE HXWinSockLib::WSAAsyncGetHostByAddr(HWND hWnd, u_int wMsg, const char FAR * addr, int len, int type, const char FAR * buf, int buflen)
{
    return m_api.m_WSAAsyncGetHostByAddr(hWnd, wMsg, addr, len, type, buf, buflen);
}

inline int HXWinSockLib::WSACancelAsyncRequest(HANDLE hAsyncTaskHandle)
{
    return m_api.m_WSACancelAsyncRequest(hAsyncTaskHandle);
}

inline int HXWinSockLib::WSAAsyncSelect(SOCKET s, HWND hWnd, u_int wMsg, long lEvent)
{
    return m_api.m_WSAAsyncSelect(s, hWnd, wMsg, lEvent);
}

inline int HXWinSockLib::WSASend(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, 
                                   LPDWORD lpNumberOfBytesSent,DWORD dwFlags,
                                   LPWSAOVERLAPPED lpOverlapped, 
                                   LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    return m_api.m_WSASend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags,
                            lpOverlapped, lpCompletionRoutine);
}

inline int HXWinSockLib::WSASendTo(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, 
                                   LPDWORD lpNumberOfBytesSent,DWORD dwFlags, 
                                   const struct sockaddr* lpTo, int iToLen, 
                                   LPWSAOVERLAPPED lpOverlapped, 
                                   LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    return m_api.m_WSASendTo(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags,
                            lpTo, iToLen, lpOverlapped, lpCompletionRoutine);
}

inline int HXWinSockLib::WSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, 
                                     LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags,  
                                     LPWSAOVERLAPPED lpOverlapped,
                                     LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    return m_api.m_WSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, 
                                lpOverlapped, lpCompletionRoutine);
}

inline int HXWinSockLib::WSARecvFrom(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, 
                                     LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, 
                                     struct sockaddr* lpFrom, LPINT lpFromlen, 
                                     LPWSAOVERLAPPED lpOverlapped,
                                     LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    return m_api.m_WSARecvFrom(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, 
                                lpFrom, lpFromlen, lpOverlapped, lpCompletionRoutine);
}

inline int HXWinSockLib::WSAIoctl(SOCKET s, DWORD dwIoControlCode, LPVOID lpvInBuffer, 
                    DWORD cbInBuffer, LPVOID lpvOutBuffer, DWORD cbOutBuffer, 
                    LPDWORD lpcbBytesReturned, LPWSAOVERLAPPED lpOverlapped, 
                    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    return m_api.m_WSAIoctl(s, dwIoControlCode, lpvInBuffer, cbInBuffer, lpvOutBuffer, 
                        cbOutBuffer, lpcbBytesReturned, lpOverlapped, lpCompletionRoutine);
}


#endif /* _WINCE */

inline int HXWinSockLib::__WSAFDIsSet(SOCKET s, fd_set FAR *fdSet)
{
    return m_api.m___WSAFDIsSet(s, fdSet);
}

//
// only available with newer versions of winsock (IPv6)
//
inline int HXWinSockLib::getaddrinfo(const char* nodename, const char* servname,
                const struct addrinfo* hints, struct addrinfo** res /*out*/)
{
    return m_api.m_getaddrinfo(nodename, servname, hints, res);
}
inline int HXWinSockLib::getnameinfo(const struct sockaddr* sa,socklen_t salen, char* host /*out*/,
                    size_t hostlen,char* serv /*out*/,size_t servlen, int flags)
{
    return m_api.m_getnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
}
inline void HXWinSockLib::freeaddrinfo(struct addrinfo* ai)
{
    m_api.m_freeaddrinfo(ai);
}

inline HX_RESULT HXLoadWinSock(IUnknown* pContext)
{
    return HXWinSockLib::Lib(pContext).Load();
}
inline void HXReleaseWinSock()
{
    HXWinSockLib::Lib().UnLoad();
}

#if !defined(WINSOCKLIB_NO_SOCKAPI_DEFINES)

//
// define library api so native function 
// call semantics can be used
//
#define DEFSOCK(api) HXWinSockLib::Lib().##api

#define connect             DEFSOCK(connect)
#define bind                DEFSOCK(bind)
#define listen              DEFSOCK(listen)
#define getsockname         DEFSOCK(getsockname)
#define accept              DEFSOCK(accept)
#define closesocket         DEFSOCK(closesocket)
#define ioctlsocket         DEFSOCK(ioctlsocket)
#define getpeername         DEFSOCK(getpeername)
#define getsockopt          DEFSOCK(getsockopt)
#define htonl               DEFSOCK(htonl)
#define htons               DEFSOCK(htons)
#define inet_ntoa           DEFSOCK(inet_ntoa)
#define inet_addr           DEFSOCK(inet_addr)
#define ntohl               DEFSOCK(ntohl)
#define ntohs               DEFSOCK(ntohs)
#define recv                DEFSOCK(recv)
#define recvfrom            DEFSOCK(recvfrom)
#define select              DEFSOCK(select)
#define send                DEFSOCK(send)
#define sendto              DEFSOCK(sendto)
#define setsockopt          DEFSOCK(setsockopt)
#define shutdown            DEFSOCK(shutdown)
#define socket              DEFSOCK(socket)
#define gethostbyaddr       DEFSOCK(gethostbyaddr)

#define gethostbyname       DEFSOCK(gethostbyname)
#define gethostname         DEFSOCK(gethostname)

#if!defined(_WINCE)
#define getservbyport       DEFSOCK(getservbyport)
#define getservbyname       DEFSOCK(getservbyname)
#define getprotobynumber    DEFSOCK(getprotobynumber)
#define getprotobyname      DEFSOCK(getprotobyname)
#endif

#define WSAStartup          DEFSOCK(WSAStartup)
#define WSACleanup          DEFSOCK(WSACleanup)
#define WSASetLastError     DEFSOCK(WSASetLastError)
#define WSAGetLastError     DEFSOCK(WSAGetLastError)

#if!defined(_WINCE)
#define WSAAsyncGetServByName       DEFSOCK(WSAAsyncGetServByName)
#define WSAAsyncGetServByPort       DEFSOCK(WSAAsyncGetServByPort)
#define WSAAsyncGetProtoByName      DEFSOCK(WSAAsyncGetProtoByName)
#define WSAAsyncGetProtoByNumber    DEFSOCK(WSAAsyncGetProtoByNumber)
#define WSAAsyncGetHostByName       DEFSOCK(WSAAsyncGetHostByName)
#define WSAAsyncGetHostByAddr       DEFSOCK(WSAAsyncGetHostByAddr)
#define WSACancelAsyncRequest       DEFSOCK(WSACancelAsyncRequest)
#define WSAAsyncSelect              DEFSOCK(WSAAsyncSelect)
#define WSASend                     DEFSOCK(WSASend)

#define WSARecv                     DEFSOCK(WSARecv)
#define WSARecvDisconnect           DEFSOCK(WSARecvDisconnect)
#define WSARecvEx                   DEFSOCK(WSARecvEx)
#define WSARecvFrom                 DEFSOCK(WSARecvFrom)
#define WSARecvMsg                  DEFSOCK(WSARecvMsg)             
#define WSASendDisconnect           DEFSOCK(WSASendDisconnect)
#define WSASendTo                   DEFSOCK(WSASendTo)
//#define WSAStringToAddress          DEFSOCK(WSAStringToAddress)
//#define WSASetService               DEFSOCK(WSASetService)
#define WSANSPIoctl                 DEFSOCK(WSANSPIoctl)
#define WSAIoctl                    DEFSOCK(WSAIoctl)
#define WSAEventSelect              DEFSOCK(WSAEventSelect)
#define WSAEnumNetworkEvents        DEFSOCK(WSAEnumNetworkEvents)
//#define WSAEnumProtocols            DEFSOCK(WSAEnumProtocols)
//#define WSADuplicateSocket          DEFSOCK(WSADuplicateSocket)
#define WSAConnect                  DEFSOCK(WSAConnect)
#define WSAAccept                   DEFSOCK(WSAAccept)

#endif

#define getaddrinfo          DEFSOCK(getaddrinfo)
#define getnameinfo          DEFSOCK(getnameinfo)
#define freeaddrinfo         DEFSOCK(freeaddrinfo)

#endif // !WINSOCKLIB_NO_SOCKAPI_DEFINES

#endif // HELIX_CONFIG_SOCKLIB_EXPLICIT_LOAD

#endif	//if !defined( HX_WINSOCKLIB_H__ )

