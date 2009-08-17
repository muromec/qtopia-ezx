/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: unix_net.h,v 1.16 2006/02/23 22:31:02 ping Exp $
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

#if !defined _HPUX && !defined _AIX43 && (!defined _MAC_UNIX || ( defined(_MAC_UNIX) && ( !defined(MAC_OS_X_VERSION_10_3) || (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_3) ) ) )
typedef unsigned int socklen_t;
#endif

#ifndef _UNIX_NET
#define _UNIX_NET

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#ifndef _VXWORKS
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#else
#include <time.h>
#include "sockLib.h"
#include <ioLib.h>
#ifndef fcntl
#define fcntl ioctl
#endif
#include <fcntl.h>
#endif
#include <netinet/in.h>
#ifndef _BEOS
#include <arpa/inet.h>
#endif
#include <errno.h>

#if defined ( _SOLARIS ) || defined ( _SUN )
#include <sys/filio.h>                  // for FIONBIO
#endif

#include "hxsocket.h"
#include "conn.h"
#include "debug.h"

#define INVALID_SOCKET -1

const int nInvalidPipe = -1;

struct IHXCallback;
class CHXSimpleList;

class unix_net : public conn 
{
public:  

  ~unix_net (void);
  
  //  call new_socket() to automatically create the correct platform
  // specific network object.  The type parameter may be either
  // HX_TCP_SOCKET or HX_UDP_SOCKET. If new_socket() returns NULL,
  // an error occurred and the conn object was not created. Call
  // last_error() to get the error
  static unix_net *new_socket (IUnknown* pContext, UINT16 type);
  
  // just for DNS for hostname...
  // is introduced for RTSP...
  virtual HX_RESULT dns_find_ip_addr (const char* host, UINT16 blocking=0);
  virtual HXBOOL      dns_ip_addr_found(HXBOOL* valid, ULONG32* addr);     
  
  // call init_drivers() to do any platform specific initialization of
  // the network drivers before calling any other functions in this
  // class
  static HX_RESULT init_drivers (void *params);
  
  // close_drivers() should close any network drivers used by the
  // program NOTE: The program MUST not make any other calls to the
  // network drivers until init_drivers() has been called 
  static HX_RESULT close_drivers (void *params);
 
  // host_to_ip_str() converts the host name to an ASCII ip address of
  // the form "XXX.XXX.XXX.XXX"
  static HX_RESULT host_to_ip_str (char *host, char *ip, UINT32 ulIPBufLen);

  // call done() when you are finished with the socket. Done() will
  // close the socket.  You may reuse the socket by calling init()
  // or connect() 
  virtual void done (void);
                
  static HX_RESULT get_host_name(char *name, int namelen);
  static HX_RESULT get_host_by_name(char *name, REF(struct hostent*) pHostent);

  virtual ULONG32 AddRef (void);

  virtual ULONG32 Release (void);

  virtual HX_RESULT init (UINT32 local_addr,
			  UINT16 port,
			  UINT16 blocking=0) =0;
  
  
  virtual HX_RESULT init_unix (UINT16 type, 
			       UINT32 local_addr,
			       UINT16 port,
			       UINT16 blocking=0);
                                                                                 
  virtual HX_RESULT listen (UINT16 backlog);

  virtual HX_RESULT listen (ULONG32 ulLocalAddr,
			    UINT16 port,
			    UINT16 backlog,
			    UINT16 blocking,
			    ULONG32 ulPlatform) =0;
 
                
  virtual HX_RESULT connect (const char *host, 
			     UINT16 port,
			     UINT16 blocking=0,
			     ULONG32 ulPlatform=0);
        
  virtual HX_RESULT connect (sockaddr_in *addr);

  virtual HX_RESULT blocking (void);

  virtual HX_RESULT CheckOnDNS(void);
  
  virtual HX_RESULT CheckForConnection(void);
              
  virtual HX_RESULT nonblocking (void);
                
  virtual HX_RESULT read (void *buf, 
			  UINT16 *size);

  virtual HX_RESULT readfrom (REF(IHXBuffer*)   pBuffer,
			      REF(UINT32)        ulAddress,
			      REF(UINT16)        ulPort);

  virtual HX_RESULT write (void *buf,
			   UINT16 *size);

  virtual HX_RESULT writeto (void *buf,
			     UINT16 *len,
			     ULONG32 addr,
			     UINT16 port);

  /* some operation doesn't make sense for TCP */
  virtual HX_RESULT join_multicast_group(ULONG32 addr, ULONG32 if_addr)
  {
    return HXR_INVALID_OPERATION;
  }
  virtual HX_RESULT leave_multicast_group(ULONG32 addr, ULONG32 if_addr)
  {
    return HXR_INVALID_OPERATION;
  }
  virtual HX_RESULT set_broadcast(HXBOOL enable)
  {
    return HXR_INVALID_OPERATION;
  }  
  virtual HX_RESULT set_multicast_if(ULONG32 ulInterface)
  {
    return HXR_INVALID_OPERATION;
  }
  virtual HX_RESULT set_send_size(ULONG32 send_size)
  {
    return HXR_INVALID_OPERATION;
  }
  virtual HX_RESULT set_multicast()
  {
      return HXR_INVALID_OPERATION;
  }
  virtual HX_RESULT set_multicast_ttl(unsigned char ttl)
  {
      return HXR_INVALID_OPERATION;
  }

  virtual HX_RESULT reuse_addr(HXBOOL enable);
  virtual HX_RESULT reuse_port(HXBOOL enable);
  
  virtual ULONG32 get_addr (void);
  virtual UINT16  get_local_port (void);

  virtual HX_RESULT accept (sockaddr_in *addr, HX_SOCKLEN_T *addrlen);
  virtual HX_RESULT connect_accept(sockaddr_in *addr);

  virtual HX_RESULT bind (sockaddr_in *addr);

  void set_callback     (IHXCallback* pCallback);
        
  HX_RESULT WriteComplete (char * Buffer, int length);
  int ReadyToWrite ();

  // Callbacks from our Async Net notifier object
  void CB_DNSComplete( int iSuccess );
  void CB_ConnectionComplete( int iSuccess );
  void CB_ReadWriteNotification( int iType );
  void CB_CloseNotification();
  void CB_NewConnectionReady(int iSuccess, unix_net* pConn);

  void add_read_request();
  static void process_idle();
  void add_select(int *, fd_set*, fd_set*, fd_set*);
  void process_select(int, fd_set*, fd_set*, fd_set*);

  HXBOOL read_pending;
  HX_RESULT ConnectAsync( const char* host, UINT16 port );
  void ContinueAsyncConnect();
  
  // Handle to any active Async DNS type operations. Usually NULL meaning
  // no such operations are active.
  // HANDLE m_hAsyncHandle;
  
  //Helper method for async debugging.
  const int GetSocketState() const
  {
    return m_SocketState;
  }
  
protected:
  unix_net (IUnknown* pContext); 

  HX_RESULT DoStartAsyncConn();
  void      CleanUpChildProc();
  
  char*      		m_pInBuffer;
  int        		callRaConnect;                                    
  static int 		InBlockingMode; 
  int        		bReadyToWrite;
  static CHXSimpleList* readers;

  typedef enum tagCONN_STATE
  {
    CONN_CLOSED = 0, // Socket not readable or writeable
    CONN_NO_CONN,    // Socket needs to be connected (TCP)
    CONN_DNS_INPROG, // DNS is in progress reads/writes will return
                     // HX_WOULDBLOCK
    CONN_DNS_FAILED, // Error doing DNS (return HXR_INVALID_HOST error
                     // next read or write)
    CONN_CONNECT_INPROG, // DNS is complete, but the connection not
                         // complete
    CONN_CONNECT_FAILED, // Error doing Connect (return error next
                         // read or write)
    CONN_OPEN,    // Socket readable/writeable
    CONN_CLOSING, // Socket is shutdown, reading might work, writing
                  // will fail
    CONN_LISTENNING, // Socket is listenning for connectionsa
    CONN_BOGUS_STATE // NOT A VALID SOCKET STATE - USED FOR RANGE
                     // CHECKING AT MOST
  } CONN_STATE;

  //Vars for Async DNS.
  UINT16 	     m_AsyncPort;
  char*   	     m_pAsyncHost;
  int 	 	     m_anDNSPipe[2];
  int    	     m_nChildProcID;
  struct sockaddr_in m_sSockAddrIn;
  HXBOOL               m_DNSOnly; //Used for DNS only objects.
  char               m_szPipeIP[20]; /* Flawfinder: ignore */ //big enough to hold "255.255.255.255"
  
#ifdef _UNIX_THREADS_SUPPORTED
  //On threaded network IO builds we are going to use a thread
  //instead of a fork to do async DNS.
  int                m_nResolved;
  IHXThread*         m_pResolver;
  static void *_ResolveIt( void* pArg );
#endif  

  CONN_STATE m_SocketState;

  // stores addr about where we are connected to currently
  ULONG32 CurrentAddr;  
  LONG32  m_lRefCount;

    HXBOOL				m_bReuseAddr;
    HXBOOL				m_bReusePort;
};


class unix_UDP: public unix_net 
{
  public :
  HX_RESULT init(UINT32 local_addr, UINT16 port, 
		 UINT16 blocking=0)
  {        
    callRaConnect = 0;
    return unix_net::init_unix( SOCK_DGRAM, local_addr, port, blocking);
  };

  HX_RESULT connect( const char* host,
		     UINT16 port,
		     UINT16  blocking=0,
		     ULONG32 ulPlatform = 0
		     );
  HX_RESULT connect( sockaddr_in *addr, UINT16 blocking=0);
  HX_RESULT join_multicast_group(ULONG32 addr, ULONG32 if_addr);
  HX_RESULT leave_multicast_group(ULONG32 addr, ULONG32 if_addr);
  HX_RESULT set_broadcast(HXBOOL enable);
  HX_RESULT set_multicast_if(ULONG32 ulInterface);
  HX_RESULT set_send_size(UINT32 send_size);
  HX_RESULT set_multicast();
  HX_RESULT set_multicast_ttl(unsigned char ttl);

  HX_RESULT listen( ULONG32 ulLocalAddr,
		    UINT16 port,
		    UINT16 backlog,
		    UINT16 blocking=1,
		    ULONG32 ulPlatform=0
		    )
  {
    // listening on a UDP port is not supported...
    return HXR_NOTIMPL;
  }
  unix_UDP(IUnknown* pContext);
};



class unix_TCP: public unix_net {
  public :
  HX_RESULT init(UINT32 local_addr, UINT16 port, 
		 UINT16 blocking=1)
  {
    return unix_net::init_unix(SOCK_STREAM, local_addr, port,  
                               blocking);
  };
  HX_RESULT connect(const char* host, UINT16 port, UINT16 blocking=0, ULONG32 ulPlatform = 0 );
  HX_RESULT connect(sockaddr_in *addr, UINT16 blocking=1);
  HX_RESULT listen (ULONG32 ulLocalAddr, UINT16 port, UINT16 backlog, UINT16 blocking=0, ULONG32 ulPlatform=0);
  unix_TCP(IUnknown* pContext);
};

inline HX_RESULT unix_net::blocking (void)
{
  int rtn;

#ifdef FIONBIO
  unsigned long nonblocking = 0;
  rtn = ::ioctl(get_sock(), FIONBIO, (char*)&nonblocking); 
#elif SO_NONBLOCK
    char nonblocking = 0;
    rtn = ::setsockopt(get_sock(),SOL_SOCKET,SO_NONBLOCK,&nonblocking,1);
#else
    rtn = ::fcntl(get_sock(), F_SETFL, ::fcntl(get_sock(), F_GETFL, 0) & ~O_NONBLOCK);
#endif

  if (rtn == 0)
    return HXR_OK;
  else if (errno == EBADF)
    return HXR_NET_SOCKET_INVALID;
  else if (errno == EINVAL)
    return HXR_INVALID_OPERATION;

  return HXR_INVALID_OPERATION;
}
    
inline HX_RESULT unix_net::nonblocking (void) 
{
  int rtn;

#ifdef FIONBIO
  unsigned long nonblocking = 1;
  rtn = ::ioctl(get_sock(), FIONBIO, (char*)&nonblocking); 
#elif SO_NONBLOCK
    char nonblocking = 1;
    rtn = ::setsockopt(get_sock(),SOL_SOCKET,SO_NONBLOCK,&nonblocking,1);
#else
    rtn = ::fcntl(get_sock(), F_SETFL, ::fcntl(get_sock(), F_GETFL, 0) | O_NONBLOCK);
#endif

  if (rtn == 0)
    return HXR_OK;
  else if (errno == EBADF)
    return HXR_NET_SOCKET_INVALID;
  else if (errno == EINVAL)
    return HXR_INVALID_OPERATION;

  return HXR_INVALID_OPERATION;
}

inline HX_RESULT unix_net::accept (sockaddr_in *addr, HX_SOCKLEN_T *addrlen)
{
  return ::accept(get_sock(), (sockaddr*)addr, addrlen);
}
                                         
inline HX_RESULT unix_net::bind (sockaddr_in *addr)
{
  return ::bind(get_sock(), (sockaddr*)addr, sizeof(addr));
}

#endif // _UNIX_NET
