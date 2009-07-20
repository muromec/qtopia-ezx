/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: conn.h,v 1.16 2007/07/06 20:43:57 jfinnecy Exp $
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

#ifdef _MACINTOSH
#pragma once
#endif

#ifndef _conn
#define	_conn

#include "hlxclib/time.h"

#include "hxtypes.h"
#include "hxresult.h"
#include "hxslist.h"
//#include "callback.h"
#include "hxmap.h"		// for CHXMapPtrToPtr class
#include "hxcom.h"
#include "hxengin.h"

struct IHXBuffer;
struct sockaddr_in;

typedef enum _NotificationType
{
    READ_NOTIFICATION	    = 0,
    WRITE_NOTIFICATION	    = 1,
    CONNECT_NOTIFICATION    = 2,
    DNS_NOTIFICATION	    = 3,
    CLOSE_NOTIFICATION	    = 4,
    ACCEPT_NOTIFICATION	    = 5,
    SEND_BUFFER_NOTIFICATION = 6
} NotificationType;

class conn;

class HXAsyncNetCallback
{
public:
    virtual HX_RESULT Func(NotificationType Type, HXBOOL bSuccess = TRUE, conn* pConn = NULL) = 0;
};

#ifndef HX_INVALID_SOCKET
#define HX_INVALID_SOCKET -1
#endif

#define HX_TCP_SOCKET 1
#define HX_UDP_SOCKET 2

#define MAX_CACHE 10

#if defined(HELIX_CONFIG_LOW_HEAP_STREAMING) && !defined(HELIX_FEATURE_NETWORK_USE_SELECT)
// We want this value to be as small as possible. Dividing by 16 brings us to
// 2048 which seems by trial and error to be optimal.
#define TCP_BUF_SIZE 			(32768>>4)
#else
// This was 32678. I assume it was supposed to be 32768. XXXJHHB
#define TCP_BUF_SIZE 			32768
#endif // HELIX_FEATURE_MIN_HEAP

typedef struct DNR_cache
{
    DNR_cache()
    {
	domainName  = NULL;	// domainName cached
    }

    ~DNR_cache()
    {
	if (domainName)
	{
	    delete [] domainName;
	}
    }

    ULONG32 	addr;				// inet address of domainName
    time_t	cachetime;			// time of caching
    UINT16 	empty;				// cache slot is empty
    char* 	domainName;	// domainName cached
} DNR_cache, *DNR_cache_ptr;

typedef struct _UDP_PACKET
{
    IHXBuffer* pBuffer;
    UINT32	ulAddress;
    UINT16	ulPort;
} UDP_PACKET;

class HXThread;

class conn 
{
public:
	static void		DestructGlobals();

/* 	conn::is_cached searched the cached DNR for host and returns its inet address 
	in  addr and 1 if it was found, or 0 if it was not found */
	static UINT16		is_cached 		(char *host,ULONG32 *addr);

/* 	conn::add_to_cache adds the host and its inet addr to the cache. If the cache is
	full, the oldest entry is replaced with the new entry*/
	static void			add_to_cache 	(char *host,ULONG32 addr);

/* 	conn::remove_from_cache removes the host name from the cache */
	static void			remove_from_cache 	(const char *host);

	static void			clear_cache();

/* 	call init_drivers() to do any platform specific initialization of the network drivers
	before calling any other functions in this class. params is a pointer to an optional
	platform specific structure needed to initialize the drivers. Simply typecast it to
	the correct struct in your platform specific version of init_drivers. The function
	will return HXR_OK if an error occurred otherwise it will return the platform
	specific error */
	
	static HX_RESULT		init_drivers 	(void *params);

/* 	close_drivers() should close any network drivers used by the program
 	NOTE: The program MUST not make any other calls to the network drivers
 	until init_drivers() has been called */

	static	HX_RESULT	close_drivers 	(void *params);

/* 	host_to_ip_str() converts the host name to an ASCII ip address of
	the form "XXX.XXX.XXX.XXX" */

	static 	HX_RESULT 	host_to_ip_str	(char *host, char *ip, UINT32 ulIPBufLen);

/* 	call new_socket() to automatically create the correct platform specific network object.
	The type parameter may be either HX_TCP_SOCKET or HX_UDP_SOCKET. If new_socket() returns 
	NULL, an error occurred and the conn object was not created. Call last_error to get the error */

	static	conn		*new_socket 	    (IUnknown* pContext, UINT16 type);

	
	// just for DNS for hostname...
	// is introduced for RTSP...
	virtual HX_RESULT	dns_find_ip_addr (const char * host, UINT16 blocking=0) = 0;
	virtual HXBOOL		dns_ip_addr_found(HXBOOL * valid, ULONG32 *addr)	= 0;	

/* 	last_error() returns the last platform specific error that occurred on this socket */
		virtual	HX_RESULT	last_error		(void)		{return mLastError;};

/* 	call done() when you are finsihed with the socket. Done() will close the socket.
	You may reuse the socket by calling init() or connect() or listen() */

	virtual void		done			(void) = 0;
	
	virtual HX_RESULT	init			(UINT32		local_addr,
										 UINT16 	port, 
										 UINT16 	blocking=0) = 0;

	virtual HX_RESULT	listen			(ULONG32	ulLocalAddr,
							 UINT16		port,
							 UINT16 	backlog,
							 UINT16		blocking,
							 ULONG32	ulPlatform) = 0;

	//////////////////////////////////////////////////////////////////
	// NOTE: 96/10/15 Brad
	// This is temporarily windows only, since it is related to
	// some experimental proxy/multicast work and is not yet needed
	// for all the platforms. It is not clear what the cross platform
	// equivilent to the sockaddr is.

#if	defined (_WINDOWS) || defined (_WIN32)
//	virtual HX_RESULT	accept			(ULONG32* pAddr) = 0;

	// we need it for dns_find_ip_addr since async stuff needs a window handle...
	virtual HX_RESULT 	SetWindowHandle(ULONG32 handle) {return HXR_OK;};
#elif	defined (_MACINTOSH)
	/* 
	 * these are mac only - required for setting up
	 * a socket to accept a new incomming connection
	 */ 
 	virtual HX_RESULT	GetEndpoint(REF(void*) pRef) {return HXR_NOTIMPL;}
	virtual HX_RESULT	SetupEndpoint(HXBOOL bWait) {return HXR_NOTIMPL;}
#endif

	virtual ULONG32	    AddRef	    (void)  = 0;

	virtual ULONG32	    Release	    (void)  = 0;

	virtual HX_RESULT	connect			(const char	*host, 
							 UINT16 	port,
							 UINT16 	blocking=0,
							 ULONG32	ulPlatform=0 ) = 0;
										 
	virtual HX_RESULT	blocking		(void) = 0;
	
	virtual HX_RESULT	nonblocking		(void) = 0;

	
	virtual HX_RESULT	read			(void 		*buf, 
							 UINT16 	*size) = 0;

	virtual HX_RESULT	readfrom		(REF(IHXBuffer*)   pBuffer,
							 REF(UINT32)	    ulAddress,
							 REF(UINT16)	    ulPort) = 0;
	
	virtual HX_RESULT	write			(void 		*buf,
							 UINT16 	*size) = 0;

	virtual HX_RESULT	writeto			(void 		*buf,
							 UINT16 	*len, 
							 ULONG32 	addr,
							 UINT16 	port) = 0;

	virtual	ULONG32		get_addr		(void) = 0;
	virtual UINT16		get_local_port		(void) = 0;

	virtual void		set_callback	(HXAsyncNetCallback* pCallback) {mCallBack = pCallback;};
	virtual HXAsyncNetCallback* get_callback()	{return mCallBack;};


/* connection_open() returns 1 if the socket is open 0 is it is closed */
	virtual  UINT16		connection_open	(void)		{return mConnectionOpen;};
	virtual  UINT16		connection_really_open	(void)	{return mConnectionOpen;};

/* get_sock() returns the socket number or -1 if the socket is invalid */
	virtual int		get_sock		(void)		{return mSock;};
	virtual void		set_sock		( int theSock ) { mSock = theSock; };

	virtual HXBOOL		set_receive_buf_size(int DesiredSize) {return FALSE;};

/* join_multicast_group() has this socket join a multicast group */
	virtual HX_RESULT	join_multicast_group 
	    (ULONG32 addr, ULONG32 if_addr = HX_INADDR_ANY) = 0;
	virtual HX_RESULT	leave_multicast_group
	    (ULONG32 addr, ULONG32 if_addr = HX_INADDR_ANY) = 0;
	virtual HX_RESULT       set_multicast() {return HXR_NOTIMPL;};
	virtual HX_RESULT       set_multicast_ttl(unsigned char ttl) {return HXR_NOTIMPL;};

/* SetSockOpt */
	virtual HX_RESULT	set_broadcast (HXBOOL enable) =0;
	virtual HX_RESULT	reuse_addr(HXBOOL enable) {return HXR_NOTIMPL;};
	virtual HX_RESULT	reuse_port(HXBOOL enable) {return HXR_NOTIMPL;};
	virtual HX_RESULT	set_multicast_if(UINT32 ulInterface) = 0;
	virtual HX_RESULT       set_send_size(UINT32 send_size) {return HXR_NOTIMPL;};

	// IHXNetworkInterfaceEnumerator support */
	static HX_RESULT EnumerateInterfaces
	    (REF(UINT32*) pulInterfaces, REF(UINT32) ulNumInterfaces);		    
	static HX_RESULT get_host_name(char *name, int namelen);
	static HX_RESULT get_host_by_name(char *name, REF(struct hostent*) pHostent);

	virtual void		SetUserThread(HXThread* pThread) {m_pUserThread = pThread;};
	virtual HXThread*	GetUserThread(void) {return m_pUserThread;};

/* get_first_connection_position returns the position of the first connection
   in the list of connections; if the list is empty the return value is
   NULL */

	static POSITION		get_first_connection_position ();

/* get_next_connection returns the conn object at POSITION and increments
   POSITION to point to the next */

	static void		get_next_connection	(POSITION& 	nextPos,
							 conn *& 	rpConn);

	static void		set_maxbandwidth (UINT32 ulBandwidth);

	// TCP max. bandwidth management
	static UINT32		bytes_to_preparetcpread(conn* pConn);
	static void		bytes_to_actualtcpread(conn* pConn, UINT32 ulBytesRead);

	UINT32			m_ulLastStarvingTime;
	static UINT32   	m_ulMaxBandwidth;

        inline void SetAsyncDNSPref(HXBOOL bNoAsyncDNS)
        {
            m_bNoAsyncDNS = bNoAsyncDNS;
        }
        inline HXBOOL GetAsyncDNSPref(void) const
        {
            return m_bNoAsyncDNS;
        }
        static inline void SetNetworkThreadingPref(HXBOOL bNetworkThreading)
        {
            m_bNetworkThreading = bNetworkThreading;
        }
        static inline HXBOOL GetNetworkThreadingPref(void)
        {
            return m_bNetworkThreading;
        }
        static inline void SetThreadedDNSPref(HXBOOL bThreadedDNS )
        {
            m_bThreadedDNS = bThreadedDNS;
        }
        static inline HXBOOL GetThreadedDNSPref(void)
        {
            return m_bThreadedDNS;
        }

        /*
         * WinSock Recvfrom() Now Returns WSAECONNRESET Instead of
         * Blocking or Timing Out (Q263823) workaround - only for win
         * UDP
         */
        virtual void IgnoreWSAECONNRESET(HXBOOL b) {}
        
	/*
	 * to run the networkthreadmainloop from select instead of the timer 
	 * implement this function in your platform specific connection class
	 * the parameters are the 3 fdset structures and timeval used in select
	 */
#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
	virtual HX_RESULT WaitforSelect(void *,void *) = 0;
	virtual HX_RESULT CheckForConnection()=0;
#endif /* HELIX_FEATURE_NETWORK_USE_SELECT */
        
protected:

/* 	Constructor NOTE: use new_socket() to create an instance of this class */
					 conn			(IUnknown* pContext);
/* 	class destructor */	
	virtual			   	~conn			(void);

		int					mSock;
		HX_RESULT			mLastError;			// last error to occur on this socket
		UINT16				mConnectionOpen;
		HXAsyncNetCallback*		mCallBack;

		static DNR_cache	mCache[MAX_CACHE];
		static UINT16 		mCacheCount;
	
	// just for DNS for hostname...
	// is introduced for RTSP...
	ULONG32				mHostIPAddr;
	HXBOOL				mHostIPValid;
	HXBOOL				mDNSDone;

	HXThread*			m_pUserThread;
	UCHAR*				m_pUDPBuffer;
	IUnknown*			m_pContext;
	
	static IHXMutex*		m_pConnectionListMutex;
protected:

/* add_connection_to_list () is called by the factory method new_socket()
   to keep track of all the sockets in the system; it should only be
   accessible to the base class since that's where the factory method resides */

	static void			add_connection_to_list	( IUnknown* pContext, conn *pConn );

/* remove_connection_from_list () is called in the base class destructor 
   to remove objects from the list of connections. */

	static void			remove_connection_from_list ( conn *pConn );

	static	conn*			actual_new_socket 	(IUnknown* pContext, UINT16 type);


	static CHXMapPtrToPtr*		mConnectionList;
					
	static HXThread*		m_pNetworkThread;

	// TCP max. bandwidth management	
	static UINT32			m_ulTCPTotalBytesRead;
	static UINT32			m_ulTCPReadTimeStamp;
	static CHXSimpleList*		m_pTCPStarvingList;

        //This member variable holds the pref for turning on/off Async
        //DNS. Async DNS, under un*x, uses a fork which screws up 'gdb'.
        //You can set NoAsyncDNS=1 in your prefs file to turn off the fork.
        HXBOOL m_bNoAsyncDNS;
        static HXBOOL m_bNetworkThreading;
        static HXBOOL m_bThreadedDNS;
};

class DestructConnGlobals
{
public:
    DestructConnGlobals() {};
    ~DestructConnGlobals() {conn::DestructGlobals();};
};

#if defined(HELIX_FEATURE_SECURECONN)
class secureconn : public conn
{
private:
    LONG32 m_lRefCount;
    conn* m_pActualConn;
    LONG32 m_FakeFD;
    static LONG32 zm_Count;
    IHXSSL*	m_pHXSSL;
    
public:
    secureconn(IUnknown* pContext, IHXSSL* pHXSSL);
    virtual ~secureconn();
    
    virtual ULONG32 AddRef(void);
    virtual ULONG32 Release(void);
    
    virtual HX_RESULT connect(const char* host, UINT16 port, UINT16 blocking=0, ULONG32 ulPlatform=0);
    virtual HX_RESULT read(void* buf, UINT16* size);
    virtual HX_RESULT write(void* buf, UINT16* size);
    
    virtual HX_RESULT blocking(void);
    virtual HX_RESULT nonblocking(void);
    virtual HX_RESULT	readfrom	(REF(IHXBuffer*)   pBuffer,
					 REF(UINT32)	    ulAddress,
					 REF(UINT16)	    ulPort);
	
    virtual HX_RESULT	writeto		(void 		*buf,
					 UINT16 	*len, 
					 ULONG32 	addr,
					 UINT16 	port);

    virtual ULONG32	get_addr	(void);
    virtual UINT16	get_local_port	(void);
    virtual HX_RESULT	dns_find_ip_addr (const char * host, UINT16 blocking=0);
    virtual HXBOOL	dns_ip_addr_found(HXBOOL * valid, ULONG32 *addr);
    virtual void	done(void);
    virtual HX_RESULT	init		(UINT32		local_addr,
					 UINT16 	port, 
					 UINT16 	blocking=0);
    virtual HX_RESULT	listen		(ULONG32	ulLocalAddr,
					 UINT16		port,
					 UINT16 	backlog,
					 UINT16		blocking,
					 ULONG32	ulPlatform);
    virtual HX_RESULT	join_multicast_group 
	    (ULONG32 addr, ULONG32 if_addr = HX_INADDR_ANY);
    virtual HX_RESULT	leave_multicast_group
	    (ULONG32 addr, ULONG32 if_addr = HX_INADDR_ANY);
    virtual HX_RESULT	set_broadcast (HXBOOL enable);
    virtual HX_RESULT	set_multicast_if(UINT32 ulInterface);
    
    // all of these functions need to be implemented so I can
    // call through to the final implemenations' code.
    
	virtual	HX_RESULT	last_error		(void);
#if	defined (_WINDOWS) || defined (_WIN32)
//	virtual HX_RESULT 	SetWindowHandle(ULONG32 handle) {return HXR_OK;};
#elif	defined (_MACINTOSH)
 	virtual HX_RESULT	GetEndpoint(REF(void*) pRef);
	virtual HX_RESULT	SetupEndpoint(HXBOOL bWait);
#endif
	virtual void		set_callback	(HXAsyncNetCallback* pCallback);
	virtual  UINT16		connection_open	(void);
	virtual int		get_sock		(void);
	virtual void		set_sock		( int theSock );
	virtual HXBOOL		set_receive_buf_size(int DesiredSize);
	virtual HX_RESULT	reuse_addr(HXBOOL enable);
	virtual HX_RESULT	reuse_port(HXBOOL enable);

#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
	    HX_RESULT	    WaitforSelect(void *,void *) {return HXR_FAIL;};
    virtual HX_RESULT CheckForConnection(){return HXR_FAIL;};
#endif // HELIX_FEATURE_NETWORK_USE_SELECT
};

class secureconnhelper
{
public:
static CHXMapPtrToPtr zm_ConnMap;
static conn* GetConn(LONG32 fakeFD);
static void SetConn(LONG32 fakeFD, conn* pConn);

// callbacks from rnssl
static long readCallback(LONG32 fakeFD, void* buff, LONG32 len);
static long writeCallback(LONG32 fakeFD, void* buff, LONG32 len);
static void closeCallback(LONG32 fakeFD);

};
#endif /* HELIX_FEATURE_SECURECONN */
	
#endif // _conn				    


