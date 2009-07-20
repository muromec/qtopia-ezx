/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxnetapi.h,v 1.17 2007/07/06 21:58:23 jfinnecy Exp $
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

#ifndef _HXNETAPI_H_
#define _HXNETAPI_H_

#if defined(_SYMBIAN)
#include "platform/symbian/hxsymbiannetapi.h"
#elif defined(_OPENWAVE)
#include "platform/openwave/hxopwavenetapi.h"
#else

#include "conn.h"
#include "hxengin.h"
#include "hxslist.h"
#include "hxpnets.h"
#include "preftran.h"

// necessary for authentication 
#include "hxauthn.h"
#include "hxathsp.h"
#include "hxcomsp.h"
#include "hxplnsp.h"

#include "hxsockcallback.h"
#include "hxnetutil.h"

struct IHXNetworkServices;
struct IHXListenSocket;
struct IHXTCPSocket;
struct IHXUDPSocket;
struct IHXInterruptState;
struct IHXInterruptSafe;



// for sockets
#if defined(_WIN32) || defined(_WINDOWS)
#include "platform/win/win_net.h"
#elif defined(_OPENWAVE)
#include "socketdefs.h"
#elif defined(__TCS__)
#include "sockio.h"
#else
#include "hlxclib/sys/socket.h"
#endif

class CByteGrowingQueue;

class HXAsyncNetCallback;
class HXMutex;
class HXEvent;

#ifdef _MACINTOSH

struct IhxQueueElement 
{
	IhxQueueElement 	*mNextElementInQueue; // link must be first member
	IUnknown			*mObject;
};

class InterruptSafeMacQueue 
{
	protected:
	
		QHdr 		mQueueHeader;
		Boolean		mDestructing;
		
	public:
	
		InterruptSafeMacQueue(void);
		~InterruptSafeMacQueue(void);	// decrements the ref on the irma nodes (via release)
		
		IUnknown * 	RemoveHead(void);
		HX_RESULT 	AddTail(IUnknown* pObject);	// increments the ref
		HX_RESULT	TransferToSimpleList(CHXSimpleList &simpleList);	// leaves the ref incremented
};
#endif // _MACINTOSH


class HXNetworkServices : public IHXNetworkServices,
			   public IHXNetworkInterfaceEnumerator
{
public:
				HXNetworkServices(IUnknown* pContext);
				~HXNetworkServices();
    /* IUnknown interface */
    STDMETHOD(QueryInterface)	    (THIS_ 
				    REFIID riid, void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	    (THIS);

    STDMETHOD_(ULONG32,Release)	    (THIS);

    /* IHXNetworkServices interface */
    STDMETHOD(CreateTCPSocket) 	    (THIS_ 
				    IHXTCPSocket** ppTCPSocket);

    STDMETHOD(CreateUDPSocket)	(THIS_
				IHXUDPSocket**    /*OUT*/     ppUDPSocket);

    STDMETHOD(CreateListenSocket)   (THIS_ 
				    IHXListenSocket** ppListenSocket);
    STDMETHOD(CreateResolver)	    (THIS_
    				    IHXResolver** ppResolver);


    /* IHXNetworkInterfaceEnumerator methods */
    STDMETHOD(EnumerateInterfaces)	(THIS_
		REF(UINT32*) pulInterfaces, REF(UINT32) ulNumInterfaces);


    void			Close();
    void			UseDrivers();
private:
    LONG32                      m_lRefCount;
    IUnknown*			m_pContext;
    HXBOOL			m_bNeedToCleanupDrivers;
#if !defined(HELIX_CONFIG_NOSTATICS)
    static UINT16		z_muNumDriverInstance;
#else
    static const UINT16		z_muNumDriverInstance;
#endif
};

class HXResolver: public IHXResolver
{
public:
    				HXResolver(HXNetworkServices* pNetworkServices);
    				~HXResolver();
                                HXResolver(HXNetworkServices* pNetworkServices,
                                            IUnknown*           pContext );
                                
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);
    STDMETHOD(Init)             (THIS_ IHXResolverResponse* pResp);
    STDMETHOD(GetHostByName)    (THIS_ const char* pHostName);

private:
    class HXResolverCallback: public HXAsyncNetCallback
    {
    public:
	HX_RESULT Func(NotificationType Type, HXBOOL bSuccess = TRUE, conn* pConn = NULL);
	HXResolver* m_pContext;
    };
    friend class HXResolverCallback;

    void    DNSDone(HXBOOL bSuccess);

    HXResolverCallback*	m_pCallback;

    LONG32                      m_lRefCount;
    IHXResolverResponse*       m_pResp;
    HXBOOL			m_bResolverPending;
    conn*			m_pData;
    HXNetworkServices*		m_pNetworkServices;
    IUnknown*                   m_pContext;
};

class HXUDPSocket : public IHXUDPSocket,
		     public IHXSetSocketOption,
		     public SocketCallbackInterface,
		     public IHXUDPMulticastInit,
		     public IHXSetPrivateSocketOption
{
public:
				HXUDPSocket(IUnknown* pContext, HXNetworkServices* pNetworkServices);
				~HXUDPSocket();
    /* IUnknown interface */
    STDMETHOD(QueryInterface)	(THIS_ 
				REFIID riid, void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

   /* SocketCallbackInterface interface */
 	enum {
		UDP_READ_COMMAND,
		UDP_WRITE_COMMAND,
		UDP_BIND_COMMAND
		};		
	STDMETHODIMP HandleCallback(INT32 theCommand, HX_RESULT theError);

    /* IHXUDPSocket interface */
    STDMETHOD(Init)		(THIS_
				ULONG32			ulAddr,
				UINT16			nPort,
				IHXUDPResponse*	pUDPResponse);

    STDMETHOD(Bind)		(THIS_
				UINT32			ulLocalAddr,
				UINT16 			nPort);

    STDMETHOD(Read)		(THIS_
				UINT16			Size);

    STDMETHOD(Write)		(THIS_
				IHXBuffer*		pBuffer);

    STDMETHOD(WriteTo)		(THIS_
    				ULONG32			ulAddr,
				UINT16			nPort,
				IHXBuffer*		pBuffer);

    STDMETHOD(GetLocalPort)	(THIS_
    				UINT16&			nPort);

    STDMETHOD(JoinMulticastGroup)	(THIS_
    					ULONG32	    ulMulticastAddr,
    					ULONG32	    ulInterfaceAddr);

    STDMETHOD(LeaveMulticastGroup)	(THIS_
    					ULONG32	    ulMulticastAddr,
    					ULONG32	    ulInterfaceAddr);

    /*
     * IHXUDPMulticastInit methods
     */
 
    STDMETHOD(InitMulticast)            (THIS_
                                         UINT8       ulTTL);

    /*
     *	IHXListenSocket methods
     */
    STDMETHOD(SetOption)		(THIS_ 
					 HX_SOCKET_OPTION option,
					 UINT32 ulValue);
    /*
     * IHXSetPrivateSocketOption methods
     */
    STDMETHOD(SetOption)		(THIS_ 
					 HX_PRIVATE_SOCKET_OPTION option,
					 UINT32 ulValue);    

    class UDPSocketCallback: public HXAsyncNetCallback
    {
    public:
	HX_RESULT Func(NotificationType Type, HXBOOL bSuccess = TRUE, conn* pConn = NULL);
	HXUDPSocket* m_pContext;
    };
    friend class UDPSocketCallback;

  private:
    IUnknown*                   m_pContext;
    HX_RESULT			DoRead();
    HX_RESULT			DoWrite();
    HXBOOL			IsSafe();
    LONG32                      m_lRefCount;
    IHXUDPResponse*		m_pUDPResponse;
    conn*			m_pData;
    struct sockaddr_in		m_sockAddr;
    HX_BITFIELD			m_bReadPending : 1;
    HX_BITFIELD			m_bInRead : 1;
    HX_BITFIELD			m_bInDoRead : 1;
    HX_BITFIELD			m_bInWrite : 1;
    UINT32			m_nRequired;
    CHXSimpleList		m_ReadBuffers;
    CHXSimpleList		m_WriteBuffers;
    IHXScheduler*		m_pScheduler;
    UDPSocketCallback*		m_pCallback;
    ScheduledSocketCallback*	m_pSchedulerReadCallback;
    ScheduledSocketCallback*	m_pSchedulerWriteCallback;
    ScheduledSocketCallback*	m_pNonInterruptReadCallback;
    UINT16			m_nDestPort;
    HX_BITFIELD			m_bInitComplete : 1;
    IHXInterruptState*		m_pInterruptState;
    IHXInterruptSafe*		m_pResponseInterruptSafe;
    IHXMutex*			m_pMutex;
    HX_BITFIELD			m_bInDestructor : 1;
    HXNetworkServices*		m_pNetworkServices;
#ifdef _MACINTOSH
	InterruptSafeMacQueue*	m_pInterruptSafeMacWriteQueue;	// only instantiated on the mac
#endif

    HXBOOL			m_bReuseAddr;
    HXBOOL			m_bReusePort;    
};

class HXTCPSocket : public IHXTCPSocket,
		     public IHXSetSocketOption,
		     public	SocketCallbackInterface,
		     public	IHXTCPSecureSocket
{
public:
				HXTCPSocket(IUnknown* pContext, HXNetworkServices* pNetworkServices);
				~HXTCPSocket();
    
    /* IUnknown interface */
    STDMETHOD(QueryInterface)   (THIS_ 
				REFIID riid, void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

   /* SocketCallbackInterface interface */
 	enum {
		TCP_READ_COMMAND,
		TCP_WRITE_COMMAND,
		TCP_BIND_COMMAND,
		TCP_READ_DONE_COMMAND,
		TCP_CONNECT_DONE_COMMAND
		};		
	STDMETHODIMP HandleCallback(INT32 theCommand, HX_RESULT theError);

    /*
     *  IHXTCPSocket methods
     */

    STDMETHOD(Init)		(THIS_
				IHXTCPResponse*    /*IN*/  pTCPResponse);

    STDMETHOD(SetResponse)	(THIS_
				IHXTCPResponse*    /*IN*/  pTCPResponse);

    STDMETHOD(Bind)		(THIS_
				UINT32			    ulLocalAddr,
				UINT16 			    nPort);

    STDMETHOD(Connect)		(THIS_
				const char*		    pDestination,
				UINT16			    nPort);

    STDMETHOD(Read)		(THIS_
				UINT16			    uSize);

    STDMETHOD(Write)		(THIS_
				IHXBuffer*		    pBuffer);

    STDMETHOD(WantWrite)	(THIS);

    STDMETHOD(GetLocalAddress)	(THIS_
				UINT32&			    lAddress);
    STDMETHOD(GetForeignAddress)(THIS_
				UINT32&			    lAddress);
    STDMETHOD(GetLocalPort)	(THIS_
    				UINT16&			    nPort);
    STDMETHOD(GetForeignPort)	(THIS_
    				UINT16&			    nPort);

    /*
     *	IHXTCPSecureSocket
     */
    
    STDMETHOD(SetSecure)		(THIS_
    					HXBOOL bSecure);
    
    /*
     *	IHXListenSocket methods
     */
    STDMETHOD(SetOption)		(THIS_ 
					 HX_SOCKET_OPTION option,
					 UINT32 ulValue);

    class TCPSocketCallback: public HXAsyncNetCallback
    {
    public:
	HX_RESULT Func(NotificationType Type, HXBOOL bSuccess = TRUE, conn* pConn = NULL);
	HXTCPSocket* m_pContext;
    };
   friend class TCPSocketCallback;

    STDMETHOD(AcceptConnection) (THIS_ conn* pNewCon);

private:
    HX_RESULT			DoRead();
    HX_RESULT			DoWrite();
    void			CloseDone();
    void			DNSDone(HXBOOL bSuccess);
    void			TransferBuffers();
    void			ConnectDone(HXBOOL bResult);
    HXBOOL			IsSafe();

    LONG32                      m_lRefCount;
    IHXTCPResponse*		m_pTCPResponse;
    conn*			m_pCtrl;
    UINT32			m_lForeignAddress;
    UINT16			m_nForeignPort : 16;
    UINT16			m_nRequired : 16;
    HX_BITFIELD			m_bReadPending : 1;
    HX_BITFIELD			m_bConnected : 1;
    HX_BITFIELD			m_bWantWritePending : 1;
    HX_BITFIELD			m_bInitComplete : 1;
    HX_BITFIELD			m_bInDestructor : 1;
    HX_BITFIELD			m_bInRead : 1;
    HX_BITFIELD			m_bInDoRead : 1;
    HX_BITFIELD			m_bInWrite : 1;
    HX_BITFIELD			m_bWriteFlushPending : 1;
    CByteGrowingQueue*		mSendTCP;
    CByteGrowingQueue*		mReceiveTCP;
    CHXSimpleList		m_PendingWriteBuffers;
    char*			m_pBuffer;
    TCPSocketCallback*		m_pCallback;
    UINT16			m_nLocalPort;
    IHXScheduler*		m_pScheduler;
    ScheduledSocketCallback*	m_pSchedulerReadCallback;
    ScheduledSocketCallback*	m_pSchedulerWriteCallback;
    ScheduledSocketCallback*	m_pNonInterruptReadCallback;
#ifdef _MACINTOSH
	ScheduledSocketCallback* 	m_pMacCommandCallback;
	InterruptSafeMacQueue*		m_pInterruptSafeMacWriteQueue;	// only instantiated on the mac
#endif
	
    IHXInterruptState*		m_pInterruptState;
    IHXInterruptSafe*		m_pResponseInterruptSafe;
    IHXMutex*			m_pMutex;
    HXNetworkServices*		m_pNetworkServices;
    IHXPreferences*		m_pPreferences;

    HXBOOL			m_bReuseAddr;
    HXBOOL			m_bReusePort;
    IUnknown*                   m_pContext;
    HXBOOL			m_bSecureSocket;
};

class HXListenSocket : public IHXListenSocket,
			public IHXSetSocketOption
{
public:
				HXListenSocket(IUnknown* pContext, HXNetworkServices* pNetworkServices);
				~HXListenSocket();

    /* IUnknown interface */
    STDMETHOD(QueryInterface)   (THIS_ 
				REFIID riid, void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     *  IHXListenSocket methods
     */

    STDMETHOD(Init)		(THIS_
				UINT32				ulLocalAddr,
				UINT16				port,
				IHXListenResponse*    /*IN*/	pListenResponse);
    /*
     *	IHXSetSocketOption methods
     */
    STDMETHOD(SetOption)		(THIS_ 
					 HX_SOCKET_OPTION option,
					 UINT32 ulValue);

    class ListenSocketCallback: public HXAsyncNetCallback
    {
    public:
	HX_RESULT Func(NotificationType Type, HXBOOL bSuccess = TRUE, conn* pConn = NULL);
	HXListenSocket* m_pContext;
    };
    friend class ListenSocketCallback;


private:
    LONG32                      m_lRefCount;
    IHXListenResponse*		m_pListenResponse;
    HXNetworkServices*		m_pNetworkServices;
    IUnknown*			m_pContext;

    conn*			m_pListenConn;
    ListenSocketCallback*	m_pCallback;
    HXBOOL			m_bReuseAddr;
    HXBOOL			m_bReusePort;
};

#endif /* defined(_SYMBIAN) */

#endif /*_HXNETAPI_H_*/
