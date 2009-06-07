#ifndef _HX_SOCKET_H_
#define _HX_SOCKET_H_

#ifndef HX_SOCKLEN_T
#if defined (_AIX42)
#define HX_SOCKLEN_T size_t
#elif defined(_AIX43) || defined(_FREEBSD6) || defined(_FREEBSD5) || defined(_FREEBSD4) || defined(_OPENBSD) ||defined(_NETBSD) || defined(_LINUX) || defined(_MAC_UNIX) || (defined (__GNUC__) && defined(_SOLARIS)) || defined(_MAC_UNIX)
#define HX_SOCKLEN_T socklen_t
#else
#define HX_SOCKLEN_T int
#endif
#endif /* HX_SOCKLEN_T */

#ifndef HX_SOCKADDR_T
#if defined _HPUX
#define HX_SOCKADDR_T void
#else
#define HX_SOCKADDR_T sockaddr
#endif
#endif /* HX_SOCKADDR_T */

#endif /* _HX_SOCKET_H_ */


