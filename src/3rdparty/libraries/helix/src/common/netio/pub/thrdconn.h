/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: thrdconn.h,v 1.16 2007/07/06 20:43:57 jfinnecy Exp $
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

#ifndef _THREADCONN_
#define _THREADCONN_

// These are the definitions of the messages our WndProc handles
#ifdef _WIN32
class CAsyncNetThread; // forward declare for Win32
#endif

//Defines for the HXMSGs.
#include "hxmsgs.h"

#include "hxslist.h"
class HXMutex;
class HXThread;
class HXEvent;
struct IHXCallback;
class CByteGrowingQueue;
struct IHXBuffer;

class ThreadedConn : public conn 
{
public:
//    static  HXThread*  GetNetworkThread(void);
//    static  void        DestroyNetworkThread(void);

			ThreadedConn		(IUnknown* pContext, UINT16 type);
	
/*  call new_socket() to automatically create the correct platform specific network object.
    The type parameter may be either HX_TCP_SOCKET or HX_UDP_SOCKET. If new_socket() returns 
    NULL, an error occurred and the conn object was not created. Call last_error() to get the error */
    
    static	ThreadedConn* new_socket 	(IUnknown* pContext, UINT16 type);

/*  call done() when you are finsihed with the socket. Done() will close the socket.
    You may reuse the socket by calling init() or connect() */
    
    virtual void        done            (void);

    virtual ULONG32	    AddRef	    (void);

    virtual ULONG32	    Release	    (void);
	    
    // just for DNS for hostname...
    // is introduced for RTSP...
    virtual HX_RESULT	dns_find_ip_addr (const char * host, UINT16 blocking=0);
    virtual HXBOOL	dns_ip_addr_found(HXBOOL * valid, ULONG32 *addr);	


    virtual HX_RESULT	init			(UINT32	local_addr,
						 UINT16 	port, 
						 UINT16 	blocking=0);

									     
    virtual HX_RESULT	listen			(ULONG32	ulLocalAddr,
						 UINT16		port,
						 UINT16 	backlog,
						 UINT16 	blocking=0,
						 ULONG32	ulPlatform=0);
	    
    virtual HX_RESULT	connect			(const char		*host, 
						 UINT16 	port,
						 UINT16 	blocking=0,
						 ULONG32	ulPlatform=0);
    
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

    virtual HX_RESULT	join_multicast_group(ULONG32 addr, ULONG32 if_addr);
    virtual HX_RESULT	leave_multicast_group(ULONG32 addr, ULONG32 if_addr);
    virtual UINT16	connection_really_open(void);

	virtual HX_RESULT   set_broadcast(HXBOOL enable);
    virtual HX_RESULT	reuse_addr(HXBOOL enable);
    virtual HX_RESULT	reuse_port(HXBOOL enable);
    virtual HX_RESULT	set_multicast_if(ULONG32 ulInterface);

    virtual ULONG32	get_addr		(void) ;
    virtual UINT16	get_local_port		(void) ;
    virtual void	IgnoreWSAECONNRESET	(HXBOOL b);
#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
	    HX_RESULT	    WaitforSelect(void *,void *) {return HXR_FAIL;};
	    HX_RESULT	    ActualAccept(ULONG32	    ulAddr,
				 ULONG32	    ulPlatform);
    virtual HX_RESULT	accept			(ULONG32 ulAddr);
    virtual HX_RESULT CheckForConnection(){return HXR_FAIL;};
#endif // HELIX_FEATURE_NETWORK_USE_SELECT

#if defined (_WINDOWS) || defined (_WIN32)
    // we need it for dns_find_ip_addr since async stuff needs a window handle...

    virtual HX_RESULT 	SetWindowHandle(ULONG32 handle);
	    
    
	    HX_RESULT 	ActuallSetWindowHandle(ULONG32 handle);
#endif
										     
    void		set_callback(HXAsyncNetCallback* pCallback);
    HXAsyncNetCallback*	get_callback() {return m_pNetCallback;};

    HX_RESULT		last_error(void);
    UINT16		connection_open(void);
    int			get_sock(void);
    void		set_sock(int theSock);

    HXBOOL		set_receive_buf_size(int DesiredSize);

    void		OnAsyncDNS(HXBOOL bResult);
    void		OnConnect(HXBOOL bResult);
    void		OnReadNotification(void);
    void		OnWriteNotification(void);
    void		OnAcceptNotification(void);

    HX_RESULT		ActualDnsFindIpAddr(const char* host, UINT16 blocking);
    HX_RESULT		ActualInit(UINT32 local_addr,UINT16 port, UINT16 blocking);
    HX_RESULT		ActualConnect(const char*   host, 
				      UINT16	    port,
				      UINT16 	    blocking,
				      ULONG32	    ulPlatform);
    HX_RESULT		ActualListen(ULONG32	    ulLocalAddr,
				     UINT16	    port,
				     UINT16	    backlog,
				     UINT16 	    blocking,
				     ULONG32	    ulPlatform);
    HX_RESULT		ActualBlocking(void);
    HX_RESULT		ActualNonBlocking(void);
    void		ActualDone(void);
    HXBOOL		ActualSetReceiveBufSize(UINT32 ulBufferSize);

    void		HandleAcceptNotification(conn* pConn);
    void		HandleDNSNotification(HXBOOL bSuccess);
    void		HandleConnectNotification(HXBOOL bSuccess);
    void		HandleCloseNotification();

    HX_RESULT		DoRead(HXBOOL bFromReadNotification = FALSE);
    void		DoWrite();
    void		DoNetworkIO();
    
    void		finaldone (void);
    void		Detached();
    HXBOOL		IsDone() { return m_bIsDone;};
    conn*		GetActualConn() {return m_pActualConn;};
    HXBOOL		m_bNetworkIOPending;

protected:
			~ThreadedConn		(void);

    HX_RESULT		ConvertNetworkError(HX_RESULT theErr);
    HX_RESULT		PostIOMessage();

#if defined (_WIN32) || defined (WIN32)
    CAsyncNetThread*	    m_pNotifier;
#endif

#ifdef THREADS_SUPPORTED
#ifdef HELIX_FEATURE_ADD_NETWORK_THREAD_SLEEP
    ULONG32		    m_ulNetworkThreadSleep;
#endif //HELIX_FEATURE_ADD_NETWORK_THREAD_SLEEP
#endif //THREADS_SUPPORTED

    LONG32		    m_lRefCount;
    conn*		    m_pActualConn;
    UINT16		    m_uSocketType;
    IHXThread*		    m_pNetworkThread;
    IHXThread*		    m_pMainAppThread;
    IHXMutex*		    m_pMutex;
    IHXEvent*		    m_pInitEvent;
    IHXEvent*		    m_pQuitEvent;
    IHXEvent*		    m_pDetachEvent;
    IHXEvent*		    m_pListenEvent;

    CHXSimpleList	    m_ReadUDPBuffers;
    CHXSimpleList	    m_WriteUDPBuffers;
    CByteGrowingQueue*	    m_pSendTCP;
    CByteGrowingQueue*	    m_pReceiveTCP;
    char*		    m_pTempBuffer;
    HXBOOL		    m_bConnected;

    ULONG32		    m_ulUserHandle;
    void*		    m_pInternalWindowHandle;
    HXBOOL		    m_bDetachPending;
    HXBOOL		    m_bIsDone;
    HXBOOL		    m_bInitialized;
    HXBOOL		    m_bOutstandingReadNotification;
    HXBOOL		    m_bOutstandingWriteNotification;
    HXBOOL		    m_bWriteFlushPending;
    HXBOOL		    m_bReadNowPending;
    HXBOOL		    m_bReadPostPendingWouldBlock;

    HXBOOL		    m_bListenning;
    CHXSimpleList*	    m_pIncommingConnections;
    HXBOOL		    m_bIgnoreWSAECONNRESET;

    class ThrConnSocketCallback : public HXAsyncNetCallback
    {
    public:
	HX_RESULT Func(NotificationType Type, HXBOOL bSuccess = TRUE, conn* pConn = NULL);
	ThreadedConn* m_pContext;
	ThrConnSocketCallback(ThreadedConn* pContext = NULL) : m_pContext(pContext) {}
    };
    friend class ThrConnSocketCallback;

    ThrConnSocketCallback*  m_pNetCallback;

    HX_RESULT SetActualConn(conn* pConn, ULONG32 ulPlatform);
    void      PostDoneAndDetach();
    class ThrdConnGenericCallback: public IHXCallback
    {
    public:
	UINT16		m_uCallbackType;
	ThreadedConn*   m_pConn;

	/* For DNS, Connect */
	CHXString	m_HostName;
	/* For DNS, Init, Connect, Listen */
	HXBOOL		m_bBlocking;
	/* For Init */
	UINT32		m_ulLocalAddr;
	/* For Init, Connect, Listen */
	UINT16		m_uPort;
	/* For SetWindowHandle, Connect, Listen */
	UINT32		m_ulHandle;

	/* For SetReceiveBufferSize */
	UINT32		m_ulBufferSize;

	/* For Listen */
	UINT16		m_uBacklog;


			ThrdConnGenericCallback(ThreadedConn* pConn, UINT16 uCallbackType);
	/*
	 *	IUnknown methods
	 */
	STDMETHOD(QueryInterface)	(THIS_
					REFIID riid,
					void** ppvObj);

	STDMETHOD_(ULONG32,AddRef)	(THIS);

	STDMETHOD_(ULONG32,Release)	(THIS);

	/*
	 *	IHXCallback methods
	 */
	STDMETHOD(Func)		(THIS);

    protected:
			    ~ThrdConnGenericCallback();

	LONG32		m_lRefCount;
    };
};

enum
{
      DNS_CALLBACK_TYPE			= 1
    , INIT_CALLBACK_TYPE		= 2
    , SETWINDOWHANDLE_CALLBACK_TYPE	= 3
    , CONNECT_CALLBACK_TYPE		= 4
    , BLOCKING_CALLBACK_TYPE		= 5
    , NONBLOCKING_CALLBACK_TYPE		= 6
    , DONE_CALLBACK_TYPE		= 7
    , SET_BUFFER_SIZE_CALLBACK_TYPE	= 8
    , LISTEN_CALLBACK_TYPE		= 9
    , ACCEPT_CALLBACK_TYPE		= 10
};

struct UDPPacketInfo
{
    UDPPacketInfo()
        : m_pBuffer(NULL),
          m_ulAddr(0),
          m_uPort(0)
        {};
    
    IHXBuffer* m_pBuffer;
    ULONG32	m_ulAddr;
    UINT16	m_uPort;
};

#endif /*_THREADCONN_*/
