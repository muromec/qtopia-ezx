/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: win_net.h,v 1.8 2006/02/07 19:21:26 ping Exp $
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

#ifndef _WIN_NET
#define	_WIN_NET

#include "hlxclib/sys/types.h"
#include <stdio.h>
#include "hlxclib/sys/socket.h"
#include "conn.h"
#include "hxmap.h"

//	Predeclare this guy
#ifndef WIN32_PLATFORM_PSPC
class CAsyncSockN;
						
class HXAsyncNetCallback;
#endif

class win_net : public conn 
{
public:
	
/*  call new_socket() to automatically create the correct platform specific network object.
    The type parameter may be either HX_TCP_SOCKET or HX_UDP_SOCKET. If new_socket() returns 
    NULL, an error occurred and the conn object was not created. Call last_error() to get the error */
    
    static	win_net	*new_socket 	(IUnknown* pContext, UINT16 type);

		win_net			(IUnknown* pContext);
		~win_net		(void);

/* call init_drivers() to do any platform specific initialization of the network drivers
    before calling any other functions in this class */
    
    static	HX_RESULT	init_drivers 	(void *params);

/*  close_drivers() should close any network drivers used by the program
    NOTE: The program MUST not make any other calls to the network drivers
    until init_drivers() has been called */

    static	HX_RESULT	close_drivers 	(void *params);

/*  host_to_ip_str() converts the host name to an ASCII ip address of
    the form "XXX.XXX.XXX.XXX" */

    static 	HX_RESULT 	host_to_ip_str	(char *host, char *ip, UINT32 ulIPBufLen);

/*  call done() when you are finsihed with the socket. Done() will close the socket.
    You may reuse the socket by calling init() or connect() */


    static HX_RESULT get_host_name(char *name, int namelen);
    static HX_RESULT get_host_by_name(char *name, REF(struct hostent*) pHostent);
    
    virtual void        done            (void);

    virtual ULONG32	    AddRef	    (void);

    virtual ULONG32	    Release	    (void);
	    
    // just for DNS for hostname...
    // is introduced for RTSP...
    virtual HX_RESULT	dns_find_ip_addr (const char * host, UINT16 blocking=0);
    virtual HXBOOL	dns_ip_addr_found(HXBOOL * valid, ULONG32 *addr);	


    virtual HX_RESULT	init			(UINT32	local_addr,
						 UINT16 	port, 
						 UINT16 	blocking=0)=0;

									     
    virtual HX_RESULT	init_win		(UINT16 type, 
						 UINT32		local_addr,
						 UINT16 	port, 
						 UINT16 	blocking=0);
									     
    virtual HX_RESULT	listen			(ULONG32	ulAddr,
						 UINT16		port,
						 UINT16		backlog,
						 UINT16		blocking = 0,
						 ULONG32	ulPlatform = 0);
	    
    virtual HX_RESULT	connect			(const char		*host, 
						 UINT16 	port,
						 UINT16 	blocking=0,
						 ULONG32	ulPlatform=0);
    
    virtual HX_RESULT	connect			(sockaddr_in *addr);

    virtual HX_RESULT	blocking		(void);
	    
    virtual HX_RESULT	nonblocking		(void);
	    
    virtual HX_RESULT	read			(void 		*buf, 
						 UINT16 	*size);

    virtual HX_RESULT	readfrom		(REF(IHXBuffer*)   pBuffer,
						 REF(UINT32)	    ulAddress,
						 REF(UINT16)	    ulPort);
    
    virtual HX_RESULT	write			(void 		*buf,
						 UINT16 	*size);

    virtual HX_RESULT	writeto			(void 		*buf,
						 UINT16 	*len, 
						 ULONG32 	addr,
						 UINT16 	port);

    virtual HXBOOL        set_receive_buf_size	(int DesiredSize);
    virtual UINT16	connection_really_open	(void)	{return m_SocketState == CONN_OPEN;};

    /* don't make sense for TCP */
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

    virtual HX_RESULT	reuse_addr(HXBOOL enable);
    virtual HX_RESULT	reuse_port(HXBOOL enable);    

    virtual ULONG32	get_addr		(void) ;
    virtual UINT16	get_local_port		(void) ;
    
    // we need it for dns_find_ip_addr since async stuff needs a window handle...
    virtual HX_RESULT 	SetWindowHandle(ULONG32 handle);

    virtual void IgnoreWSAECONNRESET(HXBOOL b) {m_bIgnoreWSAECONNRESET = b;}
#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
    virtual HX_RESULT	WaitforSelect(void *,void *);
    virtual HX_RESULT	CheckForConnection();
#endif HELIX_FEATURE_NETWORK_USE_SELECT
    
private:

    virtual HX_RESULT connect_accept		(ULONG32 ulPlatform, sockaddr_in Addr);

    char*			m_pInBuffer;

    static int			zm_LibRefCount;
    static CHXMapPtrToPtr	zm_MapObjectsToWinsockTasks;
    static CHXMapPtrToPtr	zm_WinsockTasksRefCounts;

public:
    static  int         IsWinsockAvailable(void* pObject);
    static  int         ReleaseWinsockUsage(void* pObject);
    
    HX_RESULT           WriteComplete   (char * Buffer, int length);
    int			ReadyToWrite    ();

#ifndef WIN32_PLATFORM_PSPC
    //	Callbacks from our Async Net notifier object
    void		CB_DNSComplete( int iSuccess );
    void		CB_ConnectionComplete( int iSuccess );
    void		CB_ReadWriteNotification( int iType );
    void		CB_CloseNotification();
    void		CB_AcceptNotification();

    HX_RESULT		ConnectAsync( LPCSTR host, UINT16 port );
    void		ContinueAsyncConnect();

    // Handle to any active Async DNS type operations. Usually NULL meaning
    // no such operations are active.
    HANDLE 		m_hAsyncHandle;
#endif

protected:
			win_net		(void); 

#ifndef WIN32_PLATFORM_PSPC
    HX_RESULT 		DoStartAsyncConn();
#endif
					    
    int                 callRaConnect;					
    int			InBlockingMode; 
    int                 bReadyToWrite;

    struct sockaddr 	from;
    //	NEW ASYNC DNS STUFF

    typedef enum tagCONN_STATE
    {
	CONN_CLOSED = 0,			//	Socket not readable or writeable
	CONN_NO_CONN,				//	Socket needs to be connected (TCP)
	CONN_DNS_INPROG,			//	DNS is in progress reads/writes will return HX_WOULDBLOCK
	CONN_DNS_FAILED,			//	Error doing DNS (return HXR_INVALID_HOST error next read or write)
	CONN_CONNECT_INPROG,		//	DNS is complete, but the connection not complete
	CONN_CONNECT_FAILED,		//	Error doing Connect (return error next read or write)
	CONN_OPEN,					//	Socket readable/writeable
	CONN_CLOSING,				//	Socket is shutdown, reading might work, writing will fail
	CONN_LISTENNING,			// Socket is listenning for connections

	CONN_BOGUS_STATE			//	NOT A VALID SOCKET STATE - USED FOR RANGE CHECKING AT MOST
    } CONN_STATE;

    HINSTANCE		m_hInst;
    UINT16		m_AsyncPort;
    char*		m_pAsyncHost;
    char*		m_AsyncAddress;
#ifndef WIN32_PLATFORM_PSPC
    CAsyncSockN*	m_AsyncNotifier;
#endif
    CONN_STATE		m_SocketState;
    
    // stores addr about where we are connected to currently
    ULONG32		CurrentAddr;  

    // just for DNS for hostname...
    // is introduced for RTSP...
    HXBOOL		m_DNSOnly;

    LONG32		m_lRefCount;

    // 
    HXBOOL		m_bReuseAddr;
    HXBOOL		m_bReusePort;
    HXBOOL		m_bIgnoreWSAECONNRESET;

};


class win_UDP: public win_net {
public :
    HX_RESULT	init(UINT32 local_addr, UINT16 port, 
		             UINT16 blocking=0)
    {        
	 callRaConnect = 0;
	 return win_net::init_win(SOCK_DGRAM, local_addr, port, 
		                      blocking);
    };
		win_UDP(IUnknown* pContext);
    HX_RESULT	connect(const char* host, UINT16 port, UINT16  blocking=0, ULONG32 ulPlatform = 0 );
    HX_RESULT	connect(sockaddr_in *addr, UINT16 blocking=0);
    HX_RESULT 	SetWindowHandle(ULONG32 handle);
    HX_RESULT	join_multicast_group(ULONG32 addr, ULONG32 if_addr);
    HX_RESULT	leave_multicast_group(ULONG32 addr, ULONG32 if_addr);
    HX_RESULT	set_multicast_if(ULONG32 ulInterface);    
    HX_RESULT	set_broadcast(HXBOOL enable);
    HX_RESULT   set_send_size(UINT32 send_size);
    HX_RESULT set_multicast();
    HX_RESULT set_multicast_ttl(unsigned char ttl);
    HX_RESULT	listen (ULONG32 ulLocalAddr, UINT16 port, UINT16 backlog, UINT16 blocking=0, ULONG32 ulPlatform = 0);

    HX_RESULT	GetFromInfo(ULONG32& ulAddr, UINT16& nPort);
};

class win_TCP: public win_net {
public :
    HX_RESULT		init(UINT32 local_addr, UINT16 port, 
		                 UINT16 blocking=0)
    {
	     return win_net::init_win(SOCK_STREAM, local_addr, port, 
			                      blocking);
};
			win_TCP(IUnknown* pContext);
    HX_RESULT		connect(const char* host, UINT16 port, UINT16 blocking=0, ULONG32 ulPlatform = 0 );
    HX_RESULT		connect(sockaddr_in *addr, UINT16 blocking=0);
    HX_RESULT		listen (ULONG32 ulLocalAddr, UINT16 port, UINT16 backlog, UINT16 blocking=0, ULONG32 ulPlatform = 0);
};


class WinsockManager{
public :

    WinsockManager();
    ~WinsockManager();

    void    Initialize();
    HXBOOL    IsInitialized();

private :

    HXBOOL bInitialized;
};


#endif // _WIN_NET

