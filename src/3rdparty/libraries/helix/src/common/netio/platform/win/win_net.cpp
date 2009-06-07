/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: win_net.cpp,v 1.16 2006/11/21 18:29:13 ping Exp $
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

#ifdef _WIN16
#include <stdlib.h>
#endif _WIN16

#include <stdio.h>
#include <string.h>

#include "hxslist.h"
#include "platform/win/sock.h"
#include "platform/win/hxsock.h"		//  Low level socket calls

#include "hxthread.h"
#include "hxcom.h"
#include "hxengin.h"
#include "threngin.h"

#include "platform/win/win_net.h"      //  Our declaration
#include "platform/win/casynnet.h"     //  Declaration of async socket notifier
#include "netbyte.h"
#include "hxbuffer.h"
#include "timebuff.h"
#include "hxtick.h"
#include "debugout.h"		//  Debug macros
#include "hxstrutl.h"

#include "hxheap.h"
#include "thrdconn.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif									 

static WinsockManager OnlyOneOfThisInTheWholeProgram;

//#define _LOGSMIL    1
win_net::win_net(IUnknown* pContext) 			     // PostThreadMessage
	:conn(pContext)
{
    set_sock( INVALID_SOCKET );
    mLastError	    = HXR_OK;
    callRaConnect   = 1;    
    bReadyToWrite   = 0;
    m_SocketState   = CONN_CLOSED;
#ifndef WIN32_PLATFORM_PSPC
    m_hAsyncHandle  = NULL;
#endif

    m_AsyncAddress  = NULL;
    CurrentAddr	    = 0;
    m_DNSOnly	    = FALSE;
    m_pAsyncHost    = NULL;
    InBlockingMode  = 0;
    m_lRefCount	    = 0;

    m_pInBuffer = new char[TCP_BUF_SIZE];

    m_bReuseAddr = FALSE;
    m_bReusePort = FALSE;
    m_bIgnoreWSAECONNRESET = FALSE;
}

win_net::~win_net() 
{
#ifndef WIN32_PLATFORM_PSPC
    m_AsyncNotifier = CAsyncSockN::GetCAsyncSockNotifier( m_hInst , FALSE);
    if (m_AsyncNotifier)
    {
	m_AsyncNotifier->CancelSelect( this );
    }
#endif

    m_SocketState = CONN_CLOSING;
    if ((get_sock() != INVALID_SOCKET) && sockObj)       
    {       
	if (sockObj->HXclosesocket(get_sock()))
	{
	    int code = 0;
	    code = sockObj->HXWSAGetLastError();
	}    
    }

    set_sock( INVALID_SOCKET );
    m_SocketState = CONN_CLOSED;
    mConnectionOpen = 0;

    if (m_AsyncAddress)
    {
	delete [] m_AsyncAddress;
	m_AsyncAddress = NULL;
    }

    HX_VECTOR_DELETE(m_pAsyncHost);

    delete[] m_pInBuffer;
}

win_net * win_net::new_socket(IUnknown* pContext, UINT16 type)
{
    win_net* c = NULL;

    if (sockObj)
    { 
	switch(type)
	{
	case HX_TCP_SOCKET:
	    c = new win_TCP(pContext);
	    break;
		
	case HX_UDP_SOCKET:
	    c = new win_UDP(pContext);
	    break;
	}
    }

    return(c);
}


// init_drivers() should do any network driver initialization here
// params is a pointer to a platform specfic defined struct that 
// contains an required initialization data

HX_RESULT win_net::init_drivers(void *params)
{
    if(!sockObj)
      sockObj = new CHXSock;  

    if(!sockObj)
      return(HXR_OUTOFMEMORY);

    return(HXR_OK);
}


/* 	close_drivers() should close any network drivers used by the program
 	NOTE: The program MUST not make any other calls to the network drivers
 	until init_drivers() has been called */

HX_RESULT win_net::close_drivers(void *params)
{
    if(sockObj)
    {
	sockObj->HXWSACleanup();
	delete sockObj;
	sockObj = NULL;
    }  
    return(HXR_OK);
}


HX_RESULT win_net::host_to_ip_str( char *host, char *ip, UINT32 ulIPBufLen)
{
    ULONG32		dwAddress;
    IN_ADDR		rInAddress;
    HX_RESULT	theErr;
    PHOSTENT	pHostEntry;

    theErr = HXR_OK;

    //	Let's look for this in the cache first
    if (conn::is_cached( host, &dwAddress))
    {
	//	Found it, copy the 32bit address into rInAddress w/o calling memcpy()
	rInAddress = *(IN_ADDR *)&dwAddress;
    }
    else
    {
	//	Do DNS on the host name
	if (!(pHostEntry = sockObj->HXgethostbyname( host )))
	{
	    //	Error
	    theErr = HXR_DNR;
	}

	//	Return w/o attempting any copies if there's an error
	if (HXR_OK != theErr)
	{
	    goto FuncExit;
	}
	
	//	copy the ip address into rInAddress w/o calling memcpy()
	rInAddress = *(IN_ADDR *)(pHostEntry->h_addr);
    }

    //	Convert the ULONG32 IP address into a string and copy it into ip
    SafeStrCpy( ip, sockObj->HXinet_ntoa( rInAddress ) , ulIPBufLen);

    //	Single exit point
FuncExit:
    return( theErr );
}


ULONG32 win_net::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG32 win_net::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT win_net::init_win(UINT16 type, UINT32 local_addr, 
			    UINT16 port, UINT16 blocking)
{                                        
    int            s;

    s = sockObj->HXsocket(PF_INET, type, 0);
    if (s == INVALID_SOCKET) 
    {
	mLastError = HXR_NET_SOCKET_INVALID;
	return mLastError;
    }

    InBlockingMode = 1;
	     
    {
	struct linger Ling;
	Ling.l_onoff = 0;
	Ling.l_linger = 0;
	if (callRaConnect && sockObj->HXsetsockopt(s, SOL_SOCKET, SO_LINGER, 
			     (char *)&Ling, sizeof(Ling)) != 0)
	{                 
	    if(!InBlockingMode)
		return HXR_BLOCK_CANCELED;                      
	    int code;
	    code = sockObj->HXWSAGetLastError(); 
	    mLastError = HXR_NET_SOCKET_INVALID;
	    InBlockingMode = 0;
	    return mLastError;
	}
    }
	
    if(!InBlockingMode)
	return HXR_BLOCK_CANCELED;
	    
    InBlockingMode = 0;
 
    { 
	if (sockObj->HXsetsockopt(s, SOL_SOCKET, SO_REUSEADDR, 
		                      (const char *)&m_bReuseAddr, sizeof(m_bReuseAddr)) != 0)
	{
   	    int code;
	    code = sockObj->HXWSAGetLastError(); 
	    mLastError = HXR_NET_SOCKET_INVALID;
	    return mLastError;
	}

// Reuse port is not supported on win's right now...
/*
	if (sockObj->HXsetsockopt(s, SOL_SOCKET, SO_REUSEPORT, 
		                      (const char *)&m_bReusePort, sizeof(m_bReusePort)) != 0)
	{
   	    int code;
	    code = sockObj->HXWSAGetLastError(); 
	    mLastError = HXR_NET_SOCKET_INVALID;
	    return mLastError;
	}
*/	
    }    
    
    struct sockaddr_in	SockAddr;
    memset(&SockAddr, 0, sizeof(SockAddr));
    SockAddr.sin_family = AF_INET;
    
    SockAddr.sin_addr.s_addr = sockObj->HXhtonl(local_addr);
    SockAddr.sin_port = sockObj->HXhtons(port);
    if (sockObj->HXbind(s, (sockaddr*)&SockAddr, sizeof(SockAddr)) < 0) 
    {
	//perror("bind");
	goto err;
    } 

    {
	unsigned long lMode = 1;                                                            
	if (!blocking && sockObj->HXioctlsocket(s, FIONBIO, &lMode) < 0) 
	{
	    goto err;
	}    
    }

    if (!callRaConnect)
    {
	m_SocketState = CONN_OPEN;
    }
    else
    {
	m_SocketState = CONN_NO_CONN;
    }

    set_sock( s );
    return HXR_OK;
err:
    sockObj->HXclosesocket(s);
    mLastError = HXR_NET_CONNECT;
    return mLastError;
}

HX_RESULT win_net::connect(const char* host, UINT16 port, UINT16 blocking, ULONG32 ulPlatform)
{   
    DEBUGOUTSTR( "win_net::connect()\r\n" );

    m_hInst = (HINSTANCE)ulPlatform;

    if (!blocking)
    {
#ifndef WIN32_PLATFORM_PSPC
	return( ConnectAsync( host, port ) );
#else
	HX_ASSERT(0 && "No Async connect");
#endif
    }
    else
    {
	bReadyToWrite = 0;

	if(!host)                 
	{
	    mLastError = HXR_DNR;
	    return mLastError;
	}
	    
	if(get_sock() < 0)                 
	{
	    mLastError = HXR_NET_SOCKET_INVALID;
	    return mLastError;
	}
	    
	struct in_addr addr;
        const char* pTemp = strrchr(host, '.');
        if (pTemp && isdigit(*(pTemp + 1)))
	{   /* IP address. */
	    InBlockingMode = 1;
	    addr.s_addr = sockObj->HXinet_addr(host);
	    
	    if(!InBlockingMode)
		return HXR_BLOCK_CANCELED;
	    
	    InBlockingMode = 0;
	    
	    if ((UINT)addr.s_addr == (UINT)-1) 
	    {
		mLastError = HXR_DNR;
		return mLastError;
	    }
	    m_SocketState = CONN_CONNECT_INPROG;
	} 
	else
	{   /* Host name. */
	    InBlockingMode = 1;
	    struct hostent *h = sockObj->HXgethostbyname(host);
	    
	    if(!InBlockingMode)
		return HXR_BLOCK_CANCELED;
		
	    InBlockingMode = 0;
	    if (!h || !h->h_addr ) 
	    {
		mLastError = HXR_DNR;
		return mLastError;
	    }
	    memcpy(&addr, h->h_addr, sizeof(struct in_addr)); /* Flawfinder: ignore */
	    
	    // Rahul
	    m_AsyncPort = port;
	    if (m_pAsyncHost != host)
	    {
		HX_VECTOR_DELETE(m_pAsyncHost);
		m_pAsyncHost = ::new_string(host);
	    }
	}                    
	    
	struct sockaddr_in	SockAddr;
	memset(&SockAddr, 0, sizeof(SockAddr));
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = *(long*)&addr;
	SockAddr.sin_port = sockObj->HXhtons(port);

	// this stores info about current addr 
	CurrentAddr		= *(ULONG32*)&addr;
	            
	InBlockingMode = 1;                
	
	if(callRaConnect && sockObj->HXconnect(get_sock(), (sockaddr*)&SockAddr, sizeof(SockAddr)))
	{   
	    if(!InBlockingMode)
		return HXR_BLOCK_CANCELED;
		
	    InBlockingMode = 0;
	    int code;
	    code = sockObj->HXWSAGetLastError(); 
	    if(!blocking && (code == WSAEWOULDBLOCK || code == WSAEINPROGRESS))
	    {  
	       mConnectionOpen = 1;
	       return HXR_OK;
	    }   

	    mLastError = HXR_NET_CONNECT;
	    return mLastError;
	}       
	 	  
	if(!InBlockingMode)
	    return HXR_BLOCK_CANCELED;
		           
	InBlockingMode = 0;
		          
	mConnectionOpen = 1;
	m_SocketState = CONN_OPEN; 
	bReadyToWrite = 1;
#ifdef WIN32_PLATFORM_PSPC 
	// Without asyncronous notification we cannot tolerate Asyncronous connect
	// so set non-blocking here rather than at connect
	ULONG lMode = 1;
	sockObj->HXioctlsocket(get_sock(), FIONBIO, &lMode);
	if (mCallBack)
	{
	    mCallBack->Func(CONNECT_NOTIFICATION);
	}
#endif
  return HXR_OK;
    }
}

//-------------------------------------------------
#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
HX_RESULT 
win_net::WaitforSelect(void *pE,void *pC)
{
    ThreadEngine *pEngine = (ThreadEngine *)pE;
    ThreadedConn *pProcessConn = (ThreadedConn*) pC;
    if (!pEngine)
	return HXR_FAIL;

    CHXMapPtrToPtr::Iterator ndxConn;
    CHXMapPtrToPtr* pSockMap = pEngine->GetSockMap();
    fd_set readfds,exceptfds;
    HXBOOL bHaveSock = FALSE;
    HX_RESULT theErr = HXR_OK;

    FD_ZERO(&readfds);
    FD_ZERO(&exceptfds);
    
    for (ndxConn = pSockMap->Begin(); 
	 ndxConn != pSockMap->End(); ++ndxConn)
    {
	ThreadedConn* pConn = (ThreadedConn*) (*ndxConn);
	if (pProcessConn &&
	    pConn != pProcessConn)
	    continue; // dont process other connections while closing
	if (!pConn->connection_really_open())
	    continue;
	if (!pConn->GetActualConn())
	    continue;
	unsigned int s = pConn->get_sock();
	if (s != INVALID_SOCKET)
	{
	    FD_SET(s,&readfds);
	    FD_SET(s,&exceptfds);
	    bHaveSock = TRUE;
	}
    }
    if (!bHaveSock)
	return HXR_FAIL;

    theErr = sockObj->HXselect(NULL,&readfds,NULL,NULL/*&exceptfds*/,NULL);

    if (theErr == SOCKET_ERROR)
    {
	theErr = sockObj->HXWSAGetLastError();
    }
    else
    {
	theErr = HXR_OK;
    }

    // check for read ready
    for (ndxConn = pSockMap->Begin(); 
	 ndxConn != pSockMap->End(); ++ndxConn)
    {
	ThreadedConn* pConn = (ThreadedConn*) (*ndxConn);
	if (pProcessConn &&
	    pConn != pProcessConn)
	    continue; // dont process other connections while closing
	if (!pConn->GetActualConn())
	    continue;
	int s = pConn->get_sock();
	if (s != INVALID_SOCKET)
	{
	    if (FD_ISSET(s,&readfds))
	       pConn->DoRead(TRUE);
	}
     }
    return theErr;
}

HX_RESULT  
win_net::CheckForConnection()
{
    HX_ASSERT(m_SocketState == CONN_LISTENNING);

    if (m_SocketState != CONN_LISTENNING)
	return HXR_FAIL;

    sockaddr_in addr;
    int len = sizeof(sockaddr_in);
    memset(&addr, 0, len);
    
    SOCKET sock = sockObj->HXaccept(get_sock(), (sockaddr*)&addr, &len);

    if ( sock == INVALID_SOCKET )
    {
	  int code = sockObj->HXWSAGetLastError();
	// igno all errors...  r
 	  return HXR_WOULD_BLOCK;
    }

    win_net* c = (win_net*)conn::actual_new_socket(m_pContext, HX_TCP_SOCKET);
    c->AddRef();
    conn::add_connection_to_list ( m_pContext, c );

    if ( c )
    {
	c->set_sock(sock);
	c->connect_accept((ULONG32)m_hInst, addr);
	if (mCallBack && (m_SocketState == CONN_LISTENNING))
	{
	    mCallBack->Func(ACCEPT_NOTIFICATION, TRUE, (conn*)c);
	}
    }
    HX_RELEASE(c);
    return HXR_OK;
}
#endif //defined(HELIX_FEATURE_NETWORK_USE_SELECT)


//	This function actually starts an async DNS request
//	It does it on either the first read or first write after we
#ifndef WIN32_PLATFORM_PSPC
//	call ConnectAsync();
HX_RESULT win_net::DoStartAsyncConn()
{
    DEBUGOUTSTR( "win_net::DoStartAsyncConn()\r\n" );
    
    if (!m_pAsyncHost)
    {
	return (mLastError = HXR_FAILED);
    }

    if (!m_AsyncAddress)
    {
	m_AsyncAddress = new char[MAXGETHOSTSTRUCT];
    }

    if (!m_AsyncAddress)
    {
	return (mLastError = HXR_OUTOFMEMORY);
    }

    m_AsyncNotifier = CAsyncSockN::GetCAsyncSockNotifier( m_hInst , TRUE);
    m_AsyncNotifier->DoAsyncDNS( this, m_pAsyncHost, m_AsyncAddress, MAXGETHOSTSTRUCT );
    m_SocketState = CONN_DNS_INPROG;

    return( mLastError = HXR_WOULD_BLOCK );
}

//	This method get's called by connect() in the case of an async request
//	It doesn't however actually start the connection.  It just registers
//	that we need to do the connection.  DoStartAsyncConn() will really do it.
HX_RESULT win_net::ConnectAsync( LPCSTR host, UINT16 port )
{
    DEBUGOUTSTR( "win_net::ConnectAsync()\r\n" );

    bReadyToWrite = 0;

    if (!host)                 
    {
	mLastError = HXR_DNR;
	return mLastError;
    }
	
    if (get_sock() == INVALID_SOCKET)
    {
	mLastError = HXR_NET_SOCKET_INVALID;
	return mLastError;
    }
	
    struct in_addr addr;  
    const char* pTemp = strrchr(host, '.');
    if (pTemp && atoi(pTemp + 1))
    {   /* IP address. */
	addr.s_addr = sockObj->HXinet_addr(host);
	
	if ((UINT)addr.s_addr == (UINT)-1) 
	{
	    mLastError = HXR_DNR;
	    return mLastError;
	}
	else
	{
	    LPSTR			pTemp;
	    hostent			*pHost;

	    if (!m_AsyncAddress)
	    {
		m_AsyncAddress = new char[MAXGETHOSTSTRUCT];
	    }

	    if (!m_AsyncAddress)
	    {
		mLastError = HXR_OUTOFMEMORY;
		return (mLastError);
	    }

	    pHost = (hostent *)m_AsyncAddress;

	    // this stores info about current addr 
	    CurrentAddr		= *(ULONG32 *)&addr;

	    m_AsyncPort = port;
	    pTemp = (LPSTR)&addr.s_addr;
	    pHost->h_addr_list = &pTemp;

	    CB_DNSComplete( TRUE );
	}
    } 
    else if (conn::is_cached((char *)host,(ULONG32 *) &addr))
    {
	LPSTR			pTemp;
	hostent			*pHost;

	if (!m_AsyncAddress)
	{
	    m_AsyncAddress = new char[MAXGETHOSTSTRUCT];
	}

	if (!m_AsyncAddress)
	{
	    mLastError = HXR_OUTOFMEMORY;
	    return (mLastError);
	}

	pHost = (hostent *)m_AsyncAddress;

	// this stores info about current addr 
	CurrentAddr		= *(ULONG32 *)&addr;

	m_AsyncPort = port;
	pTemp = (LPSTR)&addr.s_addr;
	pHost->h_addr_list = &pTemp;

	CB_DNSComplete( TRUE );
    }
    else
    {
	//	Save the parameters, and tell ourselves we're starting up
	m_AsyncPort = port;
	if (m_pAsyncHost != host)
	{
	    HX_VECTOR_DELETE(m_pAsyncHost);
	    m_pAsyncHost = ::new_string(host);
	}
	m_SocketState = CONN_NO_CONN;
	return(DoStartAsyncConn());
    }

    return( HXR_OK );
}

//	Once async DNS has commpleted then we'll call this guy to do the
//	connection (again asynchronously).
void win_net::ContinueAsyncConnect()
{
    hostent			*pHost;
    struct in_addr	addr;

    DEBUGOUTSTR( "win_net::ContinueAsyncConnect()\r\n" );

    pHost = (hostent *)m_AsyncAddress;

    if (!pHost)
	return;

    memcpy( &(addr.s_addr), pHost->h_addr, sizeof( addr.s_addr ) ); /* Flawfinder: ignore */
    m_AsyncNotifier = CAsyncSockN::GetCAsyncSockNotifier( m_hInst ,TRUE);
    m_AsyncNotifier->DoAsyncSelect( this );
    
    struct sockaddr_in	SockAddr;

    memset( &SockAddr, 0, sizeof( SockAddr ) );
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_addr.s_addr = *(long*)&(addr.s_addr);
    SockAddr.sin_port = sockObj->HXhtons( m_AsyncPort );
                

    if(callRaConnect && sockObj->HXconnect( get_sock(), (sockaddr*)&SockAddr, sizeof( SockAddr ) ))
    {   
	int code;
	code = sockObj->HXWSAGetLastError(); 
	if (code == WSAEWOULDBLOCK || code == WSAEINPROGRESS)
	{  
	    m_SocketState = CONN_CONNECT_INPROG;
	    mConnectionOpen = 1;
	    return;
	}   

	mLastError = HXR_NET_CONNECT;
	m_SocketState = CONN_CONNECT_FAILED;
	return;
    }       

    m_SocketState = CONN_CONNECT_INPROG;
	              
    mConnectionOpen = 1;
    return;
}

//	Called by the notifier to tell us that the DNS request completed
void win_net::CB_DNSComplete( int iSuccess )
{
    DEBUGOUTSTR( "CB_DNSComplete()\r\n" );

    mDNSDone	 = TRUE;

    if (iSuccess && m_pAsyncHost)
    {
	conn::add_to_cache(m_pAsyncHost, get_addr());
    }

    if (m_DNSOnly)
    {
	if (iSuccess)
	{
	    mHostIPValid = TRUE;
	    mHostIPAddr = get_addr();

	}
	else
	{
	    mHostIPValid = FALSE;
	}
    }
    else
    {
	if (iSuccess)
	{
	    m_SocketState = CONN_CONNECT_INPROG;
	    ContinueAsyncConnect();
	}
	else
	{
	    m_SocketState = CONN_DNS_FAILED;
	}
    }

    if (mCallBack)	 
    {
	mCallBack->Func(DNS_NOTIFICATION, iSuccess);
    }
}

//	Called by the notifier to tell us that the Connection completed
void win_net::CB_ConnectionComplete( int iSuccess )
{
    DEBUGOUTSTR( "CB_ConnectionComplete()\r\n" );

    if (iSuccess)
    {
	m_SocketState = CONN_OPEN;
	if (mCallBack)
	{
	    mCallBack->Func(CONNECT_NOTIFICATION);
	}
    }
    else
    {
	m_SocketState = CONN_CONNECT_FAILED;

	if (mCallBack)	 
	{
	    mCallBack->Func(CONNECT_NOTIFICATION, FALSE);
	}
    }
}

//	Called by the notifier when data ready for read/write
void win_net::CB_ReadWriteNotification( int iType )
{
#ifndef _DEMPSEY
	// noisy output
    DEBUGOUTSTR( "CB_ReadWriteNotification()....\r\n" );
#endif // _DEMPSEY
    //	Should do something here....
    if (mCallBack && (m_SocketState == CONN_OPEN))
    {
	if (iType == FD_WRITE)
	{
	    mCallBack->Func(WRITE_NOTIFICATION);
	}
	else if (iType == FD_READ)
	{
	    mCallBack->Func(READ_NOTIFICATION);
	}
	
	//mCallBack->callback_task( HX_UDP_CALLBACK, NULL );
    }
}

void win_net::CB_AcceptNotification()
{
    DEBUGOUTSTR( "CB_AcceptNotification()....\r\n" );
    HX_ASSERT(m_SocketState == CONN_LISTENNING);
    
    win_net* c = (win_net*)conn::actual_new_socket(m_pContext, HX_TCP_SOCKET);
    c->AddRef();
    conn::add_connection_to_list ( m_pContext, c );

    if ( c )
    {
	sockaddr_in addr;
	int len = sizeof(addr);
	SOCKET sock = sockObj->HXaccept(get_sock(), (sockaddr*)&addr, &len);
	if ( sock != INVALID_SOCKET)
	{
	    c->set_sock(sock);
	    c->connect_accept((ULONG32)m_hInst, addr);

	    if (mCallBack && (m_SocketState == CONN_LISTENNING))
	    {
		mCallBack->Func(ACCEPT_NOTIFICATION, TRUE, (conn*)c);
	    }
	}
    }
    HX_RELEASE(c);
}

void win_net::CB_CloseNotification()
{
    DEBUGOUTSTR( "CB_CloseNotification()....\r\n" );
    m_SocketState = CONN_CLOSED;
    if (mCallBack)
    {
	mCallBack->Func(CLOSE_NOTIFICATION);
    }
}
#endif

HX_RESULT win_net::connect( sockaddr_in *addr )
{
    if(callRaConnect && sockObj->HXconnect( get_sock(), (sockaddr*)addr, sizeof( addr ) ))
    {
	mLastError = HXR_NET_CONNECT;
	return mLastError;
    }                   
    
    mConnectionOpen = 1;
    return HXR_OK;     
}


HX_RESULT win_net::write(void * buf, UINT16  *len) 
{
    int got;

    DEBUGOUTSTR( "win_net::write()\r\n" );

    if (get_sock() == INVALID_SOCKET) 
    {
	// Not connected
	return( mLastError = HXR_NET_SOCKET_INVALID );
    }

    if (m_SocketState != CONN_OPEN ) //&& m_SocketState != CONN_DNS_INPROG)
    {
	//	We won't be able to write anything here, so clear this
	//	we'll return why we didn't write anything.
	*len = 0;

	switch( m_SocketState )
	{
	case CONN_DNS_INPROG:
	case CONN_CONNECT_INPROG:
	case CONN_CLOSING:
		return( mLastError = HXR_WOULD_BLOCK );

	case CONN_CLOSED:
		return( mLastError = HXR_NET_SOCKET_INVALID );

	case CONN_NO_CONN:
#ifndef WIN32_PLATFORM_PSPC
		return( DoStartAsyncConn() );
#else
        HX_ASSERT(0 && "No Async net");
        return( HXR_NET_READ );
#endif

	case CONN_DNS_FAILED:
		return( mLastError = HXR_DNR );

	case CONN_CONNECT_FAILED:
		return( mLastError = HXR_NET_CONNECT );
	default:
		//	HUH???
		assert( 0 );
		return( mLastError = HXR_NET_READ );
	};
    }
    else
    {
	got = sockObj->HXsend( get_sock(), (char *)buf, *len, 0 );
	if (got == -1)
	{	
	    int code;
	    *len = 0;
	    code = sockObj->HXWSAGetLastError();
	    // Mask the "so what?" errors
	    if (code == WSAEWOULDBLOCK || code == WSAEINPROGRESS) 
	    {
		return HXR_WOULD_BLOCK;
	    }
	    else
	    {
		mLastError = HXR_NET_WRITE;
		return mLastError;
	    }
	}

//#if defined(_DEBUG) && defined(_LOGSMIL)
//	if (::HXDebugOptionEnabled("zLogSMIL"))
//{FILE* f1 = ::fopen("e:\\temp\\foo.txt", "a+"); ::fwrite(buf, got, 1, f1);::fclose(f1);}
//#endif
	*len = got;
	return HXR_OK;
    }
}


HX_RESULT win_net::writeto(void * buf, UINT16  *len, ULONG32 addr, UINT16 port) 
{
    //sendto
    int got;

    sockaddr_in resend_addr;
    
    ::memset( &resend_addr, 0, sizeof( resend_addr ) );
    resend_addr.sin_family = AF_INET;
    resend_addr.sin_addr.s_addr = addr;
    resend_addr.sin_port = sockObj->HXhtons(port);

    got = sockObj->HXsendto(get_sock(), (char *) buf, *len, 0, (sockaddr *)&resend_addr, sizeof (resend_addr)); 
    if (got == -1)
    {	
	int code;
	*len = 0;
	code = sockObj->HXWSAGetLastError();
		// Mask the "so what?" errors
	if (code == WSAEWOULDBLOCK || code == WSAEINPROGRESS) 
	{
	    return HXR_WOULD_BLOCK;
	}
	else
	{
	    mLastError = HXR_NET_WRITE;
	    return mLastError;
	}
    }

    *len = got;
    return HXR_OK;
}


HX_RESULT win_net::WriteComplete   (char * Buffer, int length)
{
    int sent = 0;
    unsigned short cur_sent=0;

    while(sent < length)
    {
	cur_sent = length - sent;

	HX_RESULT ret = write(Buffer + sent, &cur_sent);
	if(ret != HXR_OK && ret != HXR_WOULD_BLOCK)
	    break;
	sent += cur_sent;
    }

//	m_SocketState = CONN_NO_CONN;

    if(sent < length)
    {
	mLastError = HXR_NET_WRITE;
	return mLastError;
    }

    return HXR_OK;
}

int win_net::ReadyToWrite()
{
    if(get_sock() < 0)
    {
	bReadyToWrite = 0;
	return bReadyToWrite;
    }

    if(bReadyToWrite)
	return 1;
    
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET((UINT32)get_sock(), &writefds);
    
    TIMEVAL timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if(sockObj->HXselect(0, NULL, &writefds,NULL, &timeout) == 1)
		bReadyToWrite = 1;
    
    return bReadyToWrite;        
}

void win_net::done (void)
{
#ifndef WIN32_PLATFORM_PSPC
    m_AsyncNotifier = CAsyncSockN::GetCAsyncSockNotifier( m_hInst , FALSE);
    if (m_AsyncNotifier)
    {
	m_AsyncNotifier->CancelSelect( this );
    }
#endif

    m_SocketState = CONN_CLOSING;
    if ((get_sock() != INVALID_SOCKET) && sockObj)
    {       
	if (sockObj->HXclosesocket(get_sock()))
	{
	    int code = 0;
	    code = sockObj->HXWSAGetLastError();
	}    
    }	

    set_sock( INVALID_SOCKET );
    m_SocketState = CONN_CLOSED;
    mConnectionOpen = 0;
}

inline HX_RESULT win_net::listen(ULONG32 ulLocalAddr, UINT16 port, 
				 UINT16 backlog, UINT16 blocking,
				 ULONG32 ulPlatform)
{
    m_hInst = (HINSTANCE)ulPlatform;

// accept is polled
#ifndef WIN32_PLATFORM_PSPC
    m_AsyncNotifier = CAsyncSockN::GetCAsyncSockNotifier(m_hInst ,TRUE);
    m_AsyncNotifier->DoAsyncSelect((win_net*)this );
#endif

    HX_RESULT ret = sockObj->HXlisten(get_sock(), backlog);
    if ( SUCCEEDED(ret) )
    {
	m_SocketState = CONN_LISTENNING;
	ret = HXR_OK;
    }
    else
    {
	ret = HXR_NET_SOCKET_INVALID;
    }
    return ret;
}

inline HX_RESULT win_net::blocking (void)
{
    unsigned long nonblocking = 0;
    return sockObj->HXioctlsocket(get_sock(), FIONBIO, &nonblocking); 
}
	
inline HX_RESULT win_net::nonblocking (void) 
{
    unsigned long nonblocking = 1;
    return sockObj->HXioctlsocket(get_sock(), FIONBIO, &nonblocking); 
}

HX_RESULT win_net::connect_accept(ULONG32 ulPlatform, sockaddr_in addr)
{
    m_hInst = (HINSTANCE)ulPlatform;
    
    CurrentAddr = addr.sin_addr.s_addr;

    // set up the new connection so it will use its own notifier. 
    // now we want to add this so it will recieve messages.
#ifndef WIN32_PLATFORM_PSPC
    m_AsyncNotifier = CAsyncSockN::GetCAsyncSockNotifier( m_hInst, TRUE);
    m_AsyncNotifier->DoAsyncSelect(this);
#endif

    m_SocketState = CONN_OPEN;
    mConnectionOpen = 1;

    return HXR_OK;
}

CHXMapPtrToPtr win_net::zm_MapObjectsToWinsockTasks;
CHXMapPtrToPtr win_net::zm_WinsockTasksRefCounts;
int win_net::zm_LibRefCount = 0;


HXBOOL win_net::IsWinsockAvailable(void* pObject)
{
    HXBOOL bWinSockInitedForTask = FALSE;

    // First check to see that the drivers are loaded...
    //
    // NOTE: this doesn't include initializing Winsock for 
    // the object's task...

    if(!OnlyOneOfThisInTheWholeProgram.IsInitialized())
    {
	OnlyOneOfThisInTheWholeProgram.Initialize();
    }

    if(OnlyOneOfThisInTheWholeProgram.IsInitialized())
    {
	zm_LibRefCount++;
    
    // Make sure Winsock is initialized for the object's task...

	// get current task.
#if defined( WIN32 )
	void* ulTask = (void*)GetCurrentProcessId();
#else
	void* ulTask = (void*)GetCurrentTask();
#endif

	// add map of object (http or session) to task
	zm_MapObjectsToWinsockTasks[pObject] = ulTask;

	// bump up ref count for task
	void* pVoid;
	ULONG32 TaskRefCount = 0;

	if (zm_WinsockTasksRefCounts.Lookup(ulTask,pVoid))
	{
		TaskRefCount = (ULONG32)pVoid;
	}
	TaskRefCount++;

	bWinSockInitedForTask = TRUE;

	HX_ASSERT_VALID_PTR(sockObj);
	zm_WinsockTasksRefCounts[ulTask] = (void*)TaskRefCount;
    }

    return bWinSockInitedForTask;
}

HXBOOL win_net::ReleaseWinsockUsage(void* pObject)
{
    // Only do any of this if we've first called IsWinsockAvailable()
    if (zm_LibRefCount > 0)
    {
	// get previous task from map of object (http or session) to task
	void* ulTask;

	if (zm_MapObjectsToWinsockTasks.Lookup(pObject,ulTask))
	{
	    zm_MapObjectsToWinsockTasks.RemoveKey(pObject);

	    // bump up ref count for task
	    void* pVoid;
	    ULONG32 TaskRefCount = 0;
	    if (zm_WinsockTasksRefCounts.Lookup(ulTask,pVoid))
	    {
		TaskRefCount = (ULONG32)pVoid;
	    }
	    TaskRefCount--;

	    // if 0 then call WSACleanup
	    if (TaskRefCount == 0)
	    {
		zm_WinsockTasksRefCounts.RemoveKey(ulTask);
	    }
	}

	// Decerement total ref count
	zm_LibRefCount--;
    }

    return TRUE;
}

HX_RESULT win_net::read(void * buf, UINT16 *len) 
{
    int 		got;
    static int breakpoint = 0;

    assert( buf );
    assert( len );

// This DEBUGOUTSTR is noisy.
#ifndef _DEMPSEY
    DEBUGOUTSTR( "win_net::read()\r\n" );
#endif // !_DEMPSEY

    if (get_sock() == INVALID_SOCKET || !callRaConnect) 
    {
	// Not connected
	return( mLastError = HXR_NET_SOCKET_INVALID );
    }

    //	Is the TCP socket actually connected yet?
    /* We want to read data even if we have received CloseNotification. 
     * This is because some unread data may be still be in the pipe
     * - RA 10/01/1997
     */
    if (m_SocketState != CONN_OPEN && m_SocketState != CONN_CLOSED)
    {
	//	No
	//	We won't be able to write anything here, so clear this
	//	we'll return why we didn't write anything.
	*len = 0;

	switch( m_SocketState )
	{
	case CONN_DNS_INPROG:
	case CONN_CONNECT_INPROG:
	case CONN_CLOSING:
		return( mLastError = HXR_WOULD_BLOCK );

	case CONN_CLOSED:
		return( mLastError = HXR_NET_SOCKET_INVALID );

	case CONN_NO_CONN:
#ifndef WIN32_PLATFORM_PSPC
		return( DoStartAsyncConn() );
#else
        HX_ASSERT(0 && "No Async net");
        return( HXR_NET_READ );
#endif


	case CONN_DNS_FAILED:
		return( mLastError = HXR_DNR );

	case CONN_CONNECT_FAILED:
		return( mLastError = HXR_NET_CONNECT );

	default:
		//	HUH???
		HX_ASSERT (FALSE);
		return( mLastError = HXR_NET_READ );
	};
    }
    else
    {
	//	Now we can actually do the read
	if (callRaConnect)
	{	
	    got = sockObj->HXrecv( get_sock(), (char *)buf, *len, 0 );
	}
	else
	{
	    int 		fromlen;
	    struct sockaddr 	from;
	
	    fromlen = sizeof( from );
	    got = sockObj->HXrecvfrom( get_sock(), (char *)buf, *len, 0, &from, &fromlen );
	    if (got > 0) breakpoint++;
	}

	// Did we get an error?
	if (got == SOCKET_ERROR) 
	{   
	    *len = 0;

	    if (m_SocketState == CONN_CLOSED)
	    {
		return( mLastError = HXR_NET_SOCKET_INVALID );
	    }

	    int code;

	    code = sockObj->HXWSAGetLastError();

	    //	Translate the error
	    switch (code)
	    {
	    case WSAEWOULDBLOCK:
	    case WSAEINPROGRESS:
		    return( mLastError = HXR_WOULD_BLOCK );

	    case WSAEFAULT:
	    case WSAENOTCONN:
	    case WSAENOTSOCK:
		    return( mLastError = HXR_NET_SOCKET_INVALID );

	    case WSAENETDOWN:
		    return( mLastError = HXR_GENERAL_NONET );

	    case WSAEINTR:
		    return( mLastError = HXR_BLOCK_CANCELED );

	    case WSAEMSGSIZE:
		    return( mLastError = HXR_MSG_TOOLARGE);

	    case WSAETIMEDOUT:
	    case WSAESHUTDOWN:
	    case WSAECONNABORTED:
		    return( mLastError = HXR_SERVER_DISCONNECTED );

	    case WSAECONNRESET:
		/*
		 * WinSock Recvfrom() Now Returns WSAECONNRESET Instead of Blocking or Timing Out (Q263823)
		 * workaround
		 */
		if (!m_bIgnoreWSAECONNRESET)
		{
		    return( mLastError = HXR_SERVER_DISCONNECTED );
		}
		else
		{
		    return( mLastError = HXR_WOULD_BLOCK );
		}

	    default:
		    return( mLastError = HXR_NET_READ );   // Error we don't know what to do about
	    }
	    //	Shouldn't really get here
	    return( mLastError );
	}
	else if (got == 0 && *len != 0)
	{
	    *len = 0;	

	    if (m_SocketState == CONN_CLOSED)
	    {
		return( mLastError = HXR_NET_SOCKET_INVALID );
	    }
	    else
	    {
		return( mLastError = HXR_SERVER_DISCONNECTED );
	    }
	}
	else
	{
	    //	This should be our exit point for successful read
	    *len = got;
	    return( HXR_OK );
	}
    }
}

HX_RESULT	
win_net::readfrom (REF(IHXBuffer*) pBuffer,
		   REF(UINT32)	    ulAddress,
		   REF(UINT16)	    ulPort)
{
    int 		got = 0;
    UINT16		size = 0;

#ifndef _DEMPSEY
    DEBUGOUTSTR( "win_net::readfrom()\r\n" );
#endif // !_DEMPSEY

    pBuffer = NULL;
    ulAddress = 0;
    ulPort = 0;

    if (get_sock() == INVALID_SOCKET || callRaConnect) 
    {
	// Not connected
	return( mLastError = HXR_NET_SOCKET_INVALID );
    }

    /* We want to read data even if we have received CloseNotification. 
     * This is because some unread data may be still be in the pipe
     * - RA 10/01/1997
     */
    if (m_SocketState != CONN_OPEN && m_SocketState != CONN_CLOSED)
    {
	//	No
	//	We won't be able to write anything here, so clear this
	//	we'll return why we didn't write anything.
	switch( m_SocketState )
	{
	case CONN_DNS_INPROG:
	case CONN_CONNECT_INPROG:
	case CONN_CLOSING:
		return( mLastError = HXR_WOULD_BLOCK );

	case CONN_CLOSED:
		return( mLastError = HXR_NET_SOCKET_INVALID );

	case CONN_NO_CONN:
#ifndef WIN32_PLATFORM_PSPC
		return( DoStartAsyncConn() );
#else
        HX_ASSERT(0 && "No Async net");
        return( HXR_NET_READ );
#endif

	case CONN_DNS_FAILED:
		return( mLastError = HXR_DNR );

	case CONN_CONNECT_FAILED:
		return( mLastError = HXR_NET_CONNECT );

	default:
		//	HUH???
		HX_ASSERT (FALSE);
		return( mLastError = HXR_NET_READ );
	};
    }
    else
    {
	int 			fromlen;
	struct sockaddr_in 	from;
    
	fromlen = sizeof( from );

	got = sockObj->HXrecvfrom( get_sock(), m_pInBuffer, TCP_BUF_SIZE, 0, (struct sockaddr*)&from, &fromlen );

	// Did we get an error?
	if (got == SOCKET_ERROR) 
	{   
	    if (m_SocketState == CONN_CLOSED)
	    {
		return( mLastError = HXR_NET_SOCKET_INVALID );
	    }

	    int code;

	    code = sockObj->HXWSAGetLastError();

	    //	Translate the error
	    switch (code)
	    {
	    case WSAEWOULDBLOCK:
	    case WSAEINPROGRESS:
		    return( mLastError = HXR_WOULD_BLOCK );

	    case WSAEFAULT:
	    case WSAENOTCONN:
	    case WSAENOTSOCK:
		    return( mLastError = HXR_NET_SOCKET_INVALID );

	    case WSAENETDOWN:
		    return( mLastError = HXR_GENERAL_NONET );

	    case WSAEINTR:
		    return( mLastError = HXR_BLOCK_CANCELED );

	    case WSAEMSGSIZE:
		    return( mLastError = HXR_MSG_TOOLARGE);

	    case WSAETIMEDOUT:
	    case WSAESHUTDOWN:
	    case WSAECONNABORTED:
		    return( mLastError = HXR_SERVER_DISCONNECTED );

	    case WSAECONNRESET:
		/*
		 * WinSock Recvfrom() Now Returns WSAECONNRESET Instead of Blocking or Timing Out (Q263823)
		 * workaround
		 */
		if (!m_bIgnoreWSAECONNRESET)
		{
		    return( mLastError = HXR_SERVER_DISCONNECTED );
		}
		else
		{
		    return( mLastError = HXR_WOULD_BLOCK );
		}
		    
	    default:
		    return( mLastError = HXR_NET_READ );   // Error we don't know what to do about
	    }
	    //	Shouldn't really get here
	    return( mLastError );
	}
	else if (got == 0)
	{
	    if (m_SocketState == CONN_CLOSED)
	    {
		return( mLastError = HXR_NET_SOCKET_INVALID );
	    }
	    else
	    {
		return( mLastError = HXR_SERVER_DISCONNECTED );
	    }
	}
	else
	{
	    //	This should be our exit point for successful read
	    CHXTimeStampedBuffer* pTimeBuffer = new CHXTimeStampedBuffer;
	    pTimeBuffer->AddRef();
	    pTimeBuffer->SetTimeStamp(HX_GET_TICKCOUNT_IN_USEC());
	    pTimeBuffer->Set((UCHAR*)m_pInBuffer, got);
	    pBuffer = (IHXBuffer*) pTimeBuffer;

	    ulAddress = DwToHost(from.sin_addr.s_addr);
	    ulPort = WToHost(from.sin_port);
	    
	    return( HXR_OK );
	}
    }
}
    
ULONG32 win_net::get_addr()
{
    hostent			*pHost = (hostent *)m_AsyncAddress;
    ULONG32 addr = 0;
    
    // if CurrentAddr is set, we must have passed
    // a dotted IP address...
    if (CurrentAddr)
    {
	addr = CurrentAddr;
    }
    else if (pHost)
    {
	addr = (ULONG32) (*(ULONG32*)pHost->h_addr);
    }

    return addr;
}

UINT16 win_net::get_local_port()
{
    sockaddr_in addr;
    int addr_len = sizeof addr;
    memset(&addr, 0, HX_SAFESIZE_T(addr_len));
    int ret = sockObj->HXgetsockname(get_sock(), (sockaddr*)&addr, &addr_len);
 
    return (ret < 0) ? -1: WToHost(addr.sin_port);
}


// we need it for dns_find_ip_addr since async stuff needs a window handle...
HX_RESULT win_net::SetWindowHandle(ULONG32 handle) 
{
    m_hInst = (HINSTANCE)handle; 
    return HXR_OK;
}



HX_RESULT win_net::dns_find_ip_addr(const char * host, UINT16 blocking)
{
    if(!host)                 
    {
	mLastError = HXR_DNR;
	return mLastError;
    }

    if(get_sock() < 0)                 
    {
	mLastError = HXR_NET_SOCKET_INVALID;
	return mLastError;
    }

    if (conn::is_cached((char *) host, &mHostIPAddr))
    {
	mHostIPValid = TRUE;
	mDNSDone	= TRUE;
	mLastError = HXR_OK;
	if (mCallBack)	 
	{
	    mCallBack->Func(DNS_NOTIFICATION, TRUE);
	}
	return mLastError;
    }

    const char* pTemp = strrchr(host, '.');
    if (pTemp && atoi(pTemp + 1))
    {   /* IP address. */

	struct in_addr addr;  
	mHostIPValid = FALSE;
	mHostIPAddr = 0;
	mDNSDone	= TRUE;

	addr.s_addr = sockObj->HXinet_addr(host);
	
	if ((UINT)addr.s_addr == (UINT)-1) 
	{
	    mLastError = HXR_DNR;
	    return mLastError;
	}

	mHostIPValid = TRUE;
	mHostIPAddr = *(ULONG32 *) &addr;
	conn::add_to_cache((char *) host, mHostIPAddr);

	if (mCallBack)	 
	{
	    mCallBack->Func(DNS_NOTIFICATION, TRUE);
	}
	return HXR_OK;
    } 

    if (blocking)
    {
	struct in_addr addr;  
		            
	mHostIPValid = FALSE;
	mHostIPAddr = 0;
	mDNSDone	= TRUE;

	struct hostent *h = sockObj->HXgethostbyname(host);
	
	if (!h || !h->h_addr ) 
	{
	    mLastError = HXR_DNR;
	    return mLastError;
	}

	memcpy(&addr, h->h_addr, sizeof(struct in_addr)); /* Flawfinder: ignore */

	mHostIPValid = TRUE;
	mHostIPAddr = *(ULONG32 *) &addr;
	conn::add_to_cache((char *) host, mHostIPAddr);

	if (mCallBack)	 
	{
	    mCallBack->Func(DNS_NOTIFICATION, TRUE);
	}
	return( mLastError = HXR_OK);
    }
    else
    {
	if (m_pAsyncHost != host)
	{
	    //	Save the parameters, and tell ourselves we're starting up
	    HX_VECTOR_DELETE(m_pAsyncHost);
	    m_pAsyncHost = ::new_string(host);
	}

	if (!m_AsyncAddress)
	{
	    m_AsyncAddress = new char[MAXGETHOSTSTRUCT];
	}

	if (!m_AsyncAddress)
	{
	    return (mLastError = HXR_OUTOFMEMORY);
	}

	// set the boolean variable that we are only doing async DNs and
	// do not intend to connect
#ifndef WIN32_PLATFORM_PSPC
	m_DNSOnly   = TRUE;

	m_AsyncNotifier = CAsyncSockN::GetCAsyncSockNotifier( m_hInst , TRUE);
	m_AsyncNotifier->DoAsyncDNS( this, m_pAsyncHost, m_AsyncAddress, MAXGETHOSTSTRUCT );
	m_SocketState = CONN_DNS_INPROG;
#else 
	m_DNSOnly   = FALSE;
#endif

	return( mLastError = HXR_WOULD_BLOCK );
    }
}


HXBOOL win_net::dns_ip_addr_found(HXBOOL * valid, ULONG32 *addr)
{
    if (mDNSDone)
    {
	// reset DNS only flag...
	m_DNSOnly   = FALSE;

	*valid = mHostIPValid;
	*addr  = mHostIPAddr;

	return TRUE;
    }
    else
	return FALSE;
}


HXBOOL win_net::set_receive_buf_size(int DesiredSize)
{
    int s = get_sock();
    if (s == INVALID_SOCKET) 
    {
	mLastError = HXR_NET_SOCKET_INVALID;
	return FALSE;
    }

    int RcvBufSize = 0;
    int iSize = sizeof(RcvBufSize);
    if (sockObj->HXgetsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&RcvBufSize, 
			      &iSize))
    {
	RcvBufSize = 0;
    }

    if (RcvBufSize < DesiredSize)
    {
	RcvBufSize = DesiredSize;
	if(sockObj->HXsetsockopt(s, SOL_SOCKET, SO_RCVBUF, 
				(char *)&RcvBufSize, sizeof(RcvBufSize)))
	{
	    int code;
	    code = sockObj->HXWSAGetLastError(); 
	    return FALSE;
	}
    }

    return TRUE;
}

/*
*   reuse_addr/reuse_port has to be called before a sock binds.  however, 
*   socket is not available until it binds as it is implemented.  So, set a 
*   flag here and do the actual setsockopt right before a sock binds.
*   Look in init_win().
*/
HX_RESULT	
win_net::reuse_addr(HXBOOL enable)
{
    m_bReuseAddr = enable;
    return HXR_OK;
}

HX_RESULT	
win_net::reuse_port(HXBOOL enable)
{
    m_bReusePort = enable;
    return HXR_OK;
}

HX_RESULT
win_net::get_host_name(char* name, int namelen)
{
    HX_ASSERT(sockObj);
    
    if (sockObj->HXgethostname(name, namelen) != 0)
    {
	return HXR_FAILED;
    }
    else
    {
	return HXR_OK;
    }   
}

HX_RESULT
win_net::get_host_by_name(char* name, REF(struct hostent*) h)
{
    HX_ASSERT(sockObj);
    
    h = sockObj->HXgethostbyname(name);
    if (!h || !h->h_addr_list)
    {
	return HXR_DNR;
    }
    else
    {
	return HXR_OK;
    }
}

win_UDP::win_UDP(IUnknown* pContext)
	:win_net(pContext)
{
}

HX_RESULT win_UDP::set_broadcast(HXBOOL enable)
{
	int ret;
	SOCKET s = get_sock();
	if(s == INVALID_SOCKET)
	{
		return( mLastError = HXR_NET_SOCKET_INVALID );
	}
	ret = sockObj->HXsetsockopt( s, SOL_SOCKET, SO_BROADCAST, (char*)&enable, sizeof(UINT32) );
	if(ret == -1)
		ret = HXR_BIND;
	return ret;
}

HX_RESULT 
win_UDP::set_send_size(UINT32 send_size)
{
    int s = get_sock();
    int ret = 0;
again:
    ret = sockObj->HXsetsockopt(s, SOL_SOCKET, SO_SNDBUF,
		     (char*)&send_size, sizeof(INT32));
    if (ret < 0 && send_size > 8192)
    {
	send_size >>= 1;
        goto again;
    }
    return ret;
}


HX_RESULT win_UDP::connect(const char* host, UINT16 port, UINT16 blocking, ULONG32 ulPlatform ) 
{
    HX_RESULT ret;
    if (get_sock() < 0 && (ret = init(INADDR_ANY, 0, blocking)) != HXR_OK)
    {                
	if(ret == HXR_BLOCK_CANCELED)
	    return ret;
  
	mLastError = HXR_NET_CONNECT;
	return mLastError;
    }

    return win_net::connect(host, port, blocking, ulPlatform );
}

HX_RESULT win_UDP::SetWindowHandle(ULONG32 handle)
{
    m_hInst = (HINSTANCE)handle;
#ifndef WIN32_PLATFORM_PSPC
    m_AsyncNotifier = CAsyncSockN::GetCAsyncSockNotifier( m_hInst ,TRUE);
    m_AsyncNotifier->DoAsyncSelect( this );
#endif
    return HXR_OK;		
}

HX_RESULT win_UDP::connect(sockaddr_in * addr, UINT16 blocking) 
{                
    HX_RESULT ret;
    if (get_sock() < 0 && (ret = init(INADDR_ANY, 0, blocking)) != HXR_OK)
    { 
	if(ret == HXR_BLOCK_CANCELED)
	    return ret;                                          
	  
	mLastError = HXR_NET_CONNECT;
	return mLastError;
    }

    return win_net::connect(addr);
}                                  

win_TCP::win_TCP(IUnknown* pContext)
	:win_net(pContext)
{
}

HX_RESULT win_TCP::connect(const char* host, UINT16 port, UINT16 blocking, ULONG32 ulPlatform ) 
{   
    HX_RESULT ret;
    if (get_sock() < 0 && (ret = init(INADDR_ANY, 0, blocking)) != HXR_OK)
    {    
	if(ret == HXR_BLOCK_CANCELED)
	    return ret;                                          
      
	mLastError = HXR_NET_CONNECT;
	return mLastError;
    }

    return win_net::connect(host, port, blocking, ulPlatform );
}

HX_RESULT win_TCP::connect(sockaddr_in * addr, UINT16 blocking) 
{
    HX_RESULT ret;
    if (get_sock() < 0 && (ret = init(INADDR_ANY, 0, blocking)) != HXR_OK)
    { 
	if(ret == HXR_BLOCK_CANCELED)
	    return ret;                                          
	  
	mLastError = HXR_NET_CONNECT;
	return mLastError;
    }

    return win_net::connect(addr);
}

inline HX_RESULT win_UDP::listen(ULONG32 ulLocalAddr, UINT16 port, 
				 UINT16 backlog, UINT16 blocking,
				 ULONG32 ulPlatform)
{
    return HXR_INVALID_OPERATION;
}

inline HX_RESULT win_TCP::listen(ULONG32 ulLocalAddr, UINT16 port, 
				 UINT16 backlog, UINT16 blocking,
				 ULONG32 ulPlatform)
{
    HX_RESULT ret = HXR_NET_SOCKET_INVALID;
    if ( get_sock() < 0 )
    {
	if ( ulLocalAddr == HX_INADDR_ANY )
	    ret = init(INADDR_ANY, port, blocking);
	else
	    ret = init(ulLocalAddr, port, blocking);
    }
    if ( FAILED(ret) )
    {
	if(ret == HXR_BLOCK_CANCELED)
	    return ret;
	mLastError = HXR_NET_SOCKET_INVALID;
	return mLastError;
    }

    return win_net::listen(ulLocalAddr, port, backlog, blocking, ulPlatform);
}


HX_RESULT
win_UDP::set_multicast()
{
#ifdef NO_MULTICAST
    return HXR_MULTICAST_UDP;
#else
    INT32         	ret;
    sockaddr_in addr;
    int addr_len = sizeof addr;

    if (get_sock() == INVALID_SOCKET) 
    {
        // Not connected
        return( mLastError = HXR_NET_SOCKET_INVALID );
    }

    memset(&addr, 0, HX_SAFESIZE_T(addr_len));
    ret = sockObj->HXgetsockname(get_sock(), (sockaddr*)&addr, &addr_len);

    if (ret < 0)
    {
        return HXR_MULTICAST_UDP;
    }

    ret = sockObj->HXsetsockopt(get_sock(), IPPROTO_IP, IP_MULTICAST_IF,
                       (char*) &addr.sin_addr.s_addr,
                       sizeof (addr.sin_addr.s_addr));
    if (ret < 0)
    {
	return HXR_MULTICAST_UDP;
    }
    return HXR_OK;

#endif /* NO_MULTICAST */
}

HX_RESULT
win_UDP::set_multicast_ttl(unsigned char ttl)
{
#ifdef NO_MULTICAST
    return HXR_MULTICAST_UDP;
#else
    if (get_sock() == INVALID_SOCKET) 
    {
        // Not connected
        return( mLastError = HXR_NET_SOCKET_INVALID );
    }
    
    INT32         ret;
    INT32         ttl_proxy = ttl;

    ret = sockObj->HXsetsockopt(get_sock(), IPPROTO_IP, IP_MULTICAST_TTL,
                       (char*) &ttl_proxy, sizeof (ttl_proxy));

    if (ret < 0)
    {
        return HXR_MULTICAST_UDP;
    }
    return HXR_OK;

#endif /* ! NO_MULTICAST */
}

HX_RESULT win_UDP::join_multicast_group(ULONG32 addr, ULONG32 if_addr)
{	
    int ret;
    ip_mreq		multicast_group;

    if (get_sock() == INVALID_SOCKET) 
    {
	// Not connected
	return( mLastError = HXR_NET_SOCKET_INVALID );
    }
    
    multicast_group.imr_multiaddr.s_addr = sockObj->HXhtonl(addr);
    multicast_group.imr_interface.s_addr = sockObj->HXhtonl(if_addr);
    
    ret = sockObj->HXsetsockopt(get_sock(), IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&multicast_group , sizeof (multicast_group) );
    if (ret == -1)
    {
	int err;
	err = sockObj->HXWSAGetLastError();
	return HXR_MULTICAST_JOIN;
    }
    return HXR_OK;
}

HX_RESULT win_UDP::leave_multicast_group(ULONG32 addr, ULONG32 if_addr)
{
    int ret;
    ip_mreq		multicast_group;

    if (get_sock() == INVALID_SOCKET) 
    {
	// Not connected
	return( mLastError = HXR_NET_SOCKET_INVALID );
    }

    multicast_group.imr_multiaddr.s_addr = sockObj->HXhtonl(addr);
//	multicast_group.imr_multiaddr.s_addr = sockObj->HXinet_addr("226.0.0.8");
    multicast_group.imr_interface.s_addr = sockObj->HXhtonl(if_addr);
    
    ret = sockObj->HXsetsockopt(get_sock(), IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&multicast_group , sizeof (multicast_group) );
    if (ret == -1)
    {
	return HXR_GENERAL_MULTICAST;
    }

    return HXR_OK;
}
HX_RESULT 
win_UDP::set_multicast_if(ULONG32 ulInterface)
{
    int ret;
    int s = get_sock();
    if(s == INVALID_SOCKET)
    {
	return( mLastError = HXR_NET_SOCKET_INVALID );
    }

    unsigned long addr = sockObj->HXhtonl(ulInterface);    
    ret = sockObj->HXsetsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, 
	(char*)&addr, sizeof(addr));

    if(ret == -1)
	ret = HXR_GENERAL_MULTICAST;
    return ret;
}

HX_RESULT	
win_UDP::GetFromInfo(ULONG32& ulAddr, UINT16& nPort)
{
    return HXR_OK;
}

WinsockManager::WinsockManager()
{
    bInitialized = FALSE;
    Initialize();
}


WinsockManager::~WinsockManager()
{
    conn::close_drivers(NULL);

    bInitialized = FALSE;
}

void
WinsockManager::Initialize()
{
    bInitialized = FALSE;

    HX_ASSERT(!sockObj);
    if(!sockObj)
	sockObj = new CHXSock;  

    if(conn::init_drivers(NULL) != HXR_OK)
	    return;

    if(sockObj && sockObj->WinSockAvail() && sockObj->InitWinsock())
    {
	bInitialized = TRUE;
    }

    UINT8 nWinSockVersion = sockObj->HXGetVersion();

    /*
     * Check to see what platform we are on
     */
    OSVERSIONINFO winver;
    winver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

#ifndef WIN32_PLATFORM_PSPC
    if(GetVersionEx(&winver))
    {
	if(nWinSockVersion == 2	&&
	   winver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
	   winver.dwMinorVersion == 0)
	{
	    /*
	     * Async lookup in Win 95 with WinSock2.0 installed returns the same handle for
	     * multiple requests. If there are multiple outstanding requests, there is
	     * no way to map the handle in the response to the outstansing request since
	     * all replies get mapped to one request. NT 4.0 returns different handles so
	     * name resolution works fine.
	     *
	     * We workaround this by queueing up the DNS requests.
	     */
	    sockGlobals.m_bWinSock2Suck = TRUE;
	}
    }
#endif
}

HXBOOL    
WinsockManager::IsInitialized()
{
    return bInitialized && sockObj;
}

