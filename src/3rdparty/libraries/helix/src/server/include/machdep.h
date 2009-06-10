/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: machdep.h,v 1.9 2006/01/27 00:36:29 tknox Exp $
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
#ifndef _HX_MACHDEP_H_
#define _HX_MACHDEP_H_

/* This file is a mess!! */

#include <stdarg.h>
#include <string.h>

// For consistency, bring in the common typedefs
#include "hxtypes.h"

// Everyone defines a localfilepath
void localfilepath(char* path);
// Everyone defines a gethomepath
const char* gethomepath();

#include <new.h>
#ifndef VOLATILE
#define VOLATILE __volatile__
#endif


#ifdef  _AIX
#define __aix__
#endif

#ifndef _WIN32
#if defined __FreeBSD__ || defined __linux__ || defined __hpux || \
    defined __aix__ || defined sun || defined __sgi__ || \
    defined __NetBSD__ || defined __alpha || defined __svr4__ || \
    defined __OpenBSD__ || defined _MAC_UNIX
#ifndef unix
#define unix
#endif
#endif
#endif

#if     defined unix

        void setprocstr(char *src, char **origargv, int argc, char **environ);
        #ifdef sun
        #define __sun__ 1
        #endif

        #ifdef __SVR4
        #define __svr4__ 1
        #endif

        #ifdef __alpha
        #ifndef _LINUX
        #include <sys/mode.h>
        #endif
        #endif

        #ifdef __alpha
        #undef _XOPEN_SOURCE
        #endif

        #include <unistd.h>
        #include <sys/types.h>

        #ifdef __aix__
        #include <sys/select.h>
        #include <stdlib.h>
        #endif

        #if     defined __linux && !defined _RED_HAT_5_X_
        /*
         * Linux 1.3 defines timeval as a pair of ints instead
         * of longs so we have to fake out gcc.  May/may not work
         * for Alpha.
         */
        #define int     long
        #include <linux/time.h>
        #undef  int
        #endif /* __linux */

        #if     defined __hpux
        #undef _PROTOTYPES
        #endif

        #include <sys/time.h>
        #include <time.h>

        #if     defined __hpux
        #define _PROTOTYPES
        #endif

        #include <sys/ioctl.h>
        #include <sys/stat.h>

        #if     ! (defined __alpha || defined __linux__ || defined __aix__ || defined __hpux )
        #include <sys/filio.h>
        #endif /* ! (defined __alpha || defined __linux__ || defined __aix__ || defined __hpux ) */

        #if     defined __aix__ || defined __sun__
        #include <strings.h>
        #endif

        #if     defined __alpha
        typedef char            Int8;
        typedef short int       Int16;
        typedef unsigned short int u_Int16;
        #else
        typedef char            Int8;
        typedef short int       Int16;
        typedef unsigned short int u_Int16;
        #endif /* __alpha */

        #if defined __aix__
        typedef int             tv_sec_t;
        typedef int             tv_usec_t;

        #elif defined __hpux

        typedef u_long32        tv_sec_t;
        typedef long32          tv_usec_t;
        #else

        typedef long32          tv_sec_t;
        typedef long32          tv_usec_t;

        #endif

        #if     defined __hpux
        #include <sgtty.h>
        #endif

        #if  defined __sun__
        #ifndef INADDR_NONE
                #define INADDR_NONE 0xffffffff
        #endif
        #endif


        #include <sys/socket.h>
        #include <netinet/in.h>
        #include <fcntl.h>
        #include <arpa/inet.h>
        #include <netdb.h>
        #if     0
        #include <fstream.h>
        #include <strstream.h>
        #include <iomanip.h>
        #endif  /* 0 */
        #include <errno.h>
        #include <signal.h>
        #include <sys/wait.h>

        #if     defined DEBUG
        #include <assert.h>
        #define ASSERT(x)       assert(x)
        #else
        #define ASSERT(x)       /* x */
        #endif /* defined DEBUG */

        #if defined __sgi__ || defined __hpux || (defined __sun__ && defined __svr4__)
        int killpg(pid_t pgrp, int sig);        // not valid on hpux or solaris
        inline int
        Killpg(pid_t pgrp, int sig)
        {
            return kill(pgrp, sig);
        }
        #else
        inline int
        Killpg(pid_t pgrp, int sig)
        {
            return killpg(pgrp, sig);
        }
        #endif

        #include <sys/mman.h>

#ifndef _bool_h
#define _bool_h
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#endif

int send_connection(int port, int fd);
int recv_connection(int port);

inline int
sock_error()
{
        return errno;
}

inline char*
sock_strerror(int err)
{
        return strerror(err);
}
inline int
ioctlsocket(int sock, long32 cmd, u_long32* arg) {
        return ioctl(sock, cmd, (char*)arg);
}

inline int
file_open(const char* path, int flags, mode_t mode) {
        return open(path,flags,mode);
}

inline int
process_id()
{
        return getpid();
}

#define closesocket     close
#if     defined __sun__ && !defined __svr4__
        extern int Sprintf(char *, const char *, ...);
        #define sprintf Sprintf
#endif /* defined __sun__ && !defined __svr4_ */

#if     defined __sun__ && defined __svr4__
        #include <sys/systeminfo.h>
        int gethostname(char *name,int count);
#endif /* defined __sun__ && defined __svr4_ */

#define PATH_SEP '/'
#define NEWLINE "\n"
#ifndef O_TEXT
        #define O_TEXT 0
#endif /* O_TEXT */

/* Platform Tags */
#if     defined __linux
#define PLATFORM "Linux 2.0.x"
#define LIC_PLATFORM 'A'
#elif   defined __alpha
#define PLATFORM "DEC Alpha UNIX 3.2"
#define LIC_PLATFORM 'B'
#elif   defined __bsdi__
#define PLATFORM "BSDI-2.0"
#define LIC_PLATFORM 'C'
#elif   defined __sun__
#ifdef __svr4__
#define PLATFORM "Solaris 2.5.x"
#define LIC_PLATFORM 'D'
#else
#define PLATFORM "SunOS 4.1.x"
#define LIC_PLATFORM 'E'
#endif
#elif defined __sgi__
#define PLATFORM "Irix 5.x"
#define LIC_PLATFORM 'F'
#elif defined __FreeBSD__
#define PLATFORM "FreeBSD-2.1.x"
#define LIC_PLATFORM 'G'
#elif defined hpux
#define PLATFORM "HP-UX 10.01"
#define LIC_PLATFORM 'H'
#elif defined __aix__
#define PLATFORM "AIX 4.1"
#define LIC_PLATFORM 'I'
#elif _OPENBSD
#define PLATFORM "OpenBSD 3.x"
#define LIC_PLATFORM 'P'
#elif _NETBSD
#define PLATFORM "NetBSD 1.x"
#define LIC_PLATFORM 'Q'
#endif

#elif   defined (WIN32) || defined (_WINDOWS)

        #include <stdio.h>      // for sprintf
        #include <sys/stat.h>
        #include "hlxclib/sys/socket.h"
        #include <io.h>
        #include <fcntl.h>
        #include <assert.h>
        #include <direct.h>
        #include <time.h>
        #include <process.h>
        #include <errno.h>
        #include <share.h>
#if defined (_WIN32)
        #include <wchar.h>
#endif

#if defined(_DEBUG)
// We are about to define DEBUG, so we undefine it first, to avoid a
// compiler warning.
#ifdef _WIN32
#undef DEBUG
#endif
#define DEBUG
#endif /* defined(_DEBUG) */

#ifndef NO_SERVICE
// We are about to define ASSERT, so we undefine it first, to avoid a
// compiler warning.
#ifdef _WIN32
#undef ASSERT
#endif
#define ASSERT(x)       /* x */
#endif /* NO_SERVICE */

#ifdef O_RDONLY
#undef O_RDONLY
#endif /* O_RDONLY */
#define O_RDONLY (_O_RDONLY | _O_BINARY)

#ifdef O_WRONLY
#undef O_WRONLY
#endif /* O_WRONLY */
#define O_WRONLY (_O_WRONLY | _O_BINARY)

#ifdef O_RDWR
#undef O_RDWR
#endif /* O_RDWR */
#define O_RDWR (_O_RDWR | _O_BINARY)

#ifdef O_ACCMODE
#undef O_ACCMODE
#endif /* O_ACCMODE */
#define O_ACCMODE (_O_RDONLY | _O_WRONLY | _O_RDWR | _O_BINARY)

/* This disables the 'identifier was truncated to '255' characters
 * in the debug information warning that NT generates when you use
 * STL, due to some of the STL identifiers having really long names
 */
#pragma warning( disable : 4786)

#if defined( WIN32 )
struct TCP_IO;
class  SIO;
int send_connection(TCP_IO* port, int s);
int recv_connection(SIO* stream);
#endif // defined( WIN32)


inline int
sock_error()
{
        return WSAGetLastError() - WSABASEERR;
}

inline char*
sock_strerror(int err)
{
        static char buf[256];
        sprintf(buf, "WSAerror %d", err + WSABASEERR);
        return buf;
}

inline int
process_id()
{
#if defined _WIN32
        return GetCurrentThreadId();
#else
        return 0;
#endif
}

#define WSAENODEV               (WSABASEERR + 19)
#define WSAESPIPE               (WSABASEERR + 29)

//
// Non-POSIX errors
//

#define EWOULDBLOCK             (WSAEWOULDBLOCK - WSABASEERR)           // 35
#define EINPROGRESS             (WSAEINPROGRESS - WSABASEERR)           // 36

//
// Windows-NT doesn't support SO_REUSEPORT, but it turns out that
// SO_REUSEADDR seems to include the port.
//

#ifndef SO_REUSEPORT
        #define SO_REUSEPORT             SO_REUSEADDR
#endif // !SO_REUSEPORT

//
// The following definitons were stolen from freebsd:/usr/include/sys/types.h
//

typedef long            long32;
typedef unsigned long   u_long32;
typedef char            Int8;
typedef long            Int32;
typedef short int       Int16;
typedef unsigned short int u_Int16;

#define NBBY 8
typedef long32 fd_mask;
#define NFDBITS (sizeof(fd_mask) * NBBY)

typedef unsigned short mode_t;

extern "C" int getopt(int argc, char * const *argv, const char *optstring);
int inet_aton(register const char *cp, struct in_addr* addr);
int mkdir(const char *path, mode_t mode);

class HXTime;
int gettimeofday(HXTime *, void *);
int strcasecmp(const char *, const char *);
int strncasecmp(const char *, const char *, int);

inline int
ioctl(int fd, u_long32 request, char* argp) {
        WSASetLastError(100);   // XXX use a specific code
        return -1;
}

inline int
file_open(const char* path, int flags, mode_t mode) {
        return _sopen(path,flags,_SH_DENYNO,mode);
}

extern BOOL IsService;

#define PATH_SEP '\\'
#define NEWLINE "\r\n"

/* Platform tags for version out put */
/* might need to fix this for DEC alpha NT */
#if     defined __alpha
#define PLATFORM "NT 3.5.x (DEC Alpha)"
#define LIC_PLATFORM 'K'
#else
#define PLATFORM "NT 4.0"
#define LIC_PLATFORM 'J'
#endif

typedef long32          tv_sec_t;
typedef long32          tv_usec_t;

#if defined _SERVER
extern HANDLE AcceptorMutex;
extern HANDLE ServerDoneEvents[];
extern DWORD  DoneCtr;
void BeginCriticalSection(HANDLE mutex);
void EndCriticalSection(HANDLE mutex);
#endif

#elif defined __MWERKS__

#include <assert.h>
#include <errno.h>
#include <Time.h>


#include "macFD.h"
#include "sockfd.h"
#include "mac_net.h"

#include "strcasecmp.h"
#include "macsockets.h"

// ASSERT is defined in both the Player and Server code.
#ifdef ASSERT
#undef ASSERT
#endif

#ifndef ASSERT
        #ifdef  _DEBUG
                #define ASSERT(x)                       assert(x)
        #else
                #define ASSERT(x)
        #endif
#endif

#define PATH_SEP                        ':'
#define NEWLINE                         "\n"
#define NO_EXPIRATION_DATE      0xFFFFFFFF
#define closesocket                     close
#define MAC_DEBUG                       0
#define SERVER_IDLE_TIME        999999L
#define MULTICAST                       1

#define PLATFORM                        "Macintosh"
#define LIC_PLATFORM            'M'

typedef long                    long32;
typedef unsigned long   u_long32;
typedef long32                  tv_sec_t;
typedef long32                  tv_usec_t;
typedef unsigned long   u_long;
typedef short                   Int16;
typedef unsigned short          u_Int16;
typedef long                    Int32;
typedef char                    Int8;

#define ntohl(x)        (x)
#define ntohs(x)        (x)
#define htonl(x)        (x)
#define htons(x)        (x)

#ifndef SOCKET_ERROR
        #define SOCKET_ERROR -1
#endif

#ifndef INADDR_NONE
        #define INADDR_NONE -1
#endif


#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

/*
 *      Change the current directory.
 */
extern int chdir(const char *path);

inline int
sock_error()
{
        return errno;
}

inline int
chdir(const char *dir)
{
        return -1;
}

inline
int process_id(void)
{
        return 57;
}

extern void terminate(int code);

int gettimeofday(struct timeval* tv, void *unused);
int setfdlimit(int limit);
// I'm declaring this deal at the top because now everyone has to implement it.
// void localfilepath(char* path);
char *strdup(char *s);

#if 0
inline int
ioctlsocket(int sock, long32 cmd, u_long32* arg) {
        return ioctl(sock, cmd, (char*)arg);
}
#endif

inline void bcopy(const void *src, void *dst, size_t len)
{
        ::BlockMove(src,dst,len);
}



inline int killpg(int pgrp, int sig) { return 0; }      // not valid on Macintosh


#else /* end of defined __MWERKS__ */
#error  Must put system dependent header files here
#endif /* defined unix (way at the top) */

#if     ! ((defined unix && defined __alpha) || defined __linux__)
/*
 * Return r where s <= r < s + m and r mod m == 0
 */
#undef roundup
inline int
roundup(int s, int m) {
        return ((s + m - 1) / m) * m;
}
#endif /* ! (defined __alpha || defined __linux__) */

#if defined unix
inline void
seconds_sleep(int seconds)
{
    sleep(seconds);
}

#elif defined _WIN32
inline void
seconds_sleep(int seconds)
{
    Sleep(seconds * 1000);
}
#elif defined _WINDOWS
inline void
seconds_sleep(int seconds)
{
    //sleep(seconds * 1000);
}

#elif defined __MWERKS__
void
inline seconds_sleep(int seconds)
{
        EventRecord theEvent;
        ::WaitNextEvent(nullEvent, &theEvent, seconds * 60, NULL);

}

#else
#error Need system dependent sleep() call
#endif

#ifdef NO_MULTICAST

/*
 * Any systems that don't have multicast capability should define this.
 * I setup some stuff so that multicast classes return errors on systems
 * that have NO_MULTICAST defined.
 */

#define IP_MULTICAST_IF                 0
#define IP_MULTICAST_TTL                0
#define IP_ADD_MEMBERSHIP               0
#define IP_DROP_MEMBERSHIP              0
struct ip_mreq {
    struct      in_addr imr_multiaddr;
    struct      in_addr imr_interface;
};

#endif /* NO_MULTICAST */

#endif/*_HX_MACHDEP_H_*/
