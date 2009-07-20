/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ot_net.h,v 1.9 2007/07/06 21:57:58 jfinnecy Exp $
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

#pragma once

#ifndef _OT_NET
#define	_OT_NET

#include "conn.h"
#ifndef _MAC_MACHO
#include <OpenTransport.h>
#include <OpenTptInternet.h>
#endif

#define kBufSize 256

typedef struct OTSvcInfo 		// Open Transport Internet services provider info 
{		
	InetSvcRef ref;				// provider reference 
	Boolean complete;			// true when asynch operation has completed 
	OTResult result;			// result code 
	void *cookie;				// cookie 
} OTSvcInfo;

#if 0
enum
{
	netOpenDriverErr =		100,
	netOpenStreamErr,
	netLostConnectionErr,	
	netDNRErr,				
	netTruncatedErr	
};
#endif

// CB Added
typedef struct MyOTInfo
{
	const char*  hostName;
	InetHostInfo hostInfo;
	InetSvcRef   inetSvcRef;
	Boolean      complete;
} MyOTInfo;

struct OTCallbackInfo
{
    OTCallbackInfo(OTEventCode code, OTResult result, void* cookie)
    {
        m_Code 	 = code;
        m_Result = result; 
        m_Cookie = cookie;
    }
    
    OTEventCode m_Code;
    OTResult    m_Result; 
    void*       m_Cookie;
};

class CHXSimpleList;

class OT_net : public conn {

public:

/* 	call new_socket() to automatically create the correct platform specific network object.
	The type parameter may be either HX_TCP_SOCKET or HX_UDP_SOCKET. If new_socket() returns 
	NULL, an error occurred and the conn object was not created. Call last_error() to get the error */
	
	static	OT_net		*new_socket 	(UINT16 type);

				   		~OT_net			(void);

/* 	call init_drivers() to do any platform specific initialization of the network drivers
	before calling any other functions in this class */
	
	static	HX_RESULT	init_drivers 	(void *params);


/* 	close_drivers() should close any network drivers used by the program
 	NOTE: The program MUST not make any other calls to the network drivers
 	until init_drivers() has been called */

	static	HX_RESULT	close_drivers 	(void *params);

/* 	host_to_ip_str() converts the host name to an ASCII ip address of
	the form "XXX.XXX.XXX.XXX" */
	static 	HX_RESULT 	host_to_ip_str	(char *host, char *ip, UINT32 ulIPBufLen);

/* 	call done() when you are finished with the socket. Done() will close the socket.
	You may reuse the socket by calling init() or connect() */
	
/* join_multicast_group() has this socket join a multicast group */
	// Note: Only the UDP subclass object supports multicast
	virtual HX_RESULT	join_multicast_group 
	    (ULONG32 addr, ULONG32 if_addr = HX_INADDR_ANY) {return HXR_INVALID_OPERATION;};
	virtual HX_RESULT	leave_multicast_group
	    (ULONG32 addr, ULONG32 if_addr = HX_INADDR_ANY) {return HXR_INVALID_OPERATION;};
	
	virtual HX_RESULT	set_broadcast(HXBOOL enable) {return HXR_INVALID_OPERATION;};
	virtual HX_RESULT	set_multicast_if(UINT32 ulInterface){return HXR_INVALID_OPERATION;};

	virtual void		done			(void);
	
	virtual ULONG32	    AddRef	    (void);

	virtual ULONG32	    Release	    (void);

	virtual HX_RESULT	init			(UINT32		local_addr,
							 UINT16 	port, 
							 UINT16 	blocking=0) = 0;
										 
	virtual HX_RESULT	listen			(ULONG32	ulLocalAddr,
							 UINT16		port,
							 UINT16 	backlog,
							 UINT16		blocking,
							 ULONG32	ulPlatform) = 0;
	
	virtual HX_RESULT	connect			(const char	*host, 
							 UINT16 	port,
							 UINT16 	blocking=0,
							 ULONG32	ulPlatform=0 ) = 0;
	
	virtual HX_RESULT	blocking		(void);
	
	virtual HX_RESULT	nonblocking		(void);
	
	virtual HX_RESULT	read			(void 		*buf, 
							 UINT16 	*size) = 0;

	virtual HX_RESULT	readfrom		(REF(IHXBuffer*)   pBuffer,
							 REF(UINT32)	    ulAddress,
							 REF(UINT16)	    ulPort) = 0;
	
	virtual HX_RESULT	write			(void 		*buf,
							 UINT16 	*size) = 0;

	virtual HX_RESULT	writeto			(void 		*buf,
							 UINT16 	*size, 
							 ULONG32 	addr,
							 UINT16 	port) {return HXR_INVALID_OPERATION;};

	virtual	ULONG32		get_addr		(void) {return mRemoteHost;};
	virtual	UINT16		get_local_port		(void) {return mLocalPort;};

											 									
// Open Tranport Stuff

/* NetHaveOT() returns true if Open Transport TCP is installed and available */
	static	Boolean		NetHaveOT 		(void);

/* NetInit  initializes the Open Transport drivers */
	static  HX_RESULT 	NetInit 		(void);

/* 	NetInit  looks up the IP address of host, and returns the IP address in addr
	if the host string contains :port, then the port is returned in port 
	else the default port specified by defaultPort is returned in port */

	static	HX_RESULT	lookup_host 	(char *name, 
										 ULONG32 *addr, 
										 UINT16 *port,
										 UINT16 defaultPort = 0);

	virtual HX_RESULT	dns_find_ip_addr (const char * host, UINT16 blocking=0);
	virtual HXBOOL		dns_ip_addr_found(HXBOOL * valid, ULONG32 *addr);	
static	HX_RESULT 			NetGetMyAddr 	(unsigned long *addr);
static  HX_RESULT           NetGetIfaceInfo(InetInterfaceInfo* ifaceInfo);
	

protected:
						OT_net			(void);

	static	Boolean 	sCheckedOT;				// have we checked already for OT
	static	Boolean 	sHaveOT;				// TRUE: OT and OT TCP are present
	static 	Boolean 	sMacAddressValid;
	static 	Boolean 	sMacAddressStrValid;
	static 	ULONG32 	sMacAddress;
	static  char 		sMacAddrStr[16]; /* Flawfinder: ignore */

// Open Tranport Stuff

			HX_RESULT	OTWait 			(void);

	static	HX_RESULT	OTSvcWait 		(OTSvcInfo *svcInfo);

	static pascal void 	NotifyProc      (OT_net *s, 
								    	 OTEventCode code,
								    	 OTResult result, 
								    	 void *cookie);

	static pascal void 	SvcNotifyProc 	(void *stream, 
										 OTEventCode code,
										 OTResult result, 
										 void *cookie );

	static pascal void MyOTNotifyProc
	(
		void* 		contextPtr, 
		OTEventCode 	code,
		OTResult 	result, 
		void* 		cookie
	);
	
	static pascal void UDPTCPNotifyProc
	(
		void* 		contextPtr, 
		OTEventCode 	code,
		OTResult 	result, 
		void* 		cookie
	);
	

	static 	HX_RESULT 	OpenInetServices (OTSvcInfo *svcInfo);


static	HX_RESULT 			NetGetMyAddrStr (char *addrStr);
  		HX_RESULT 			NetNameToAddr 	(char *name, 
										 unsigned short defaultPort, 
										 unsigned long *addr, 
										 unsigned short *port);

static	HX_RESULT			NetAddrToName 	(unsigned long addr, 
										 InetDomainName name);

			char		*mActiveName;
			ULONG32		mRemoteHost;		// IP address of server
			UINT16		mRemotePort;		// port number on server

			UINT16 		mLocalPort;				// local port number */
			LONG32 		mResponseCode;			// last response code received on stream 
			Boolean 	mClosing;				// true when closing stream 
			Boolean 	mOtherSideHasClosed;	// true when other side has closed its end of the stream 
			Boolean 	mWeHaveClosed;			// true when we have closed our end of the stream 
			Boolean 	mRelease;				// true when stream should be released 
			Boolean		mStartedReceivingData;  // used by Release to determine if ok to OTSndOrderlyDisconnect 
			Boolean		mDataArrived;			// flag to indicate data is waiting to be read
			Boolean		mDataFlowOn;			// flag to indicate data flow control is currently on
			
			EndpointRef mRef;					// endpoint reference 
			Boolean 	mComplete;				// true when asynch operation has completed 
			OTEventCode mCode;					// event code 
			OTResult 	mResult;				// result 
			void 		*mCookie;				// cookie 
			
			char 		mBuf[kBufSize];			// data buffer /* Flawfinder: ignore */

			HX_RESULT	mAsyncError;
			LONG32		m_lRefCount;
			
			CHXSimpleList*	m_pPendingCallbackListOne;
			CHXSimpleList*	m_pPendingCallbackListTwo;
			HXBOOL		m_bUsingListOne;
			HXBOOL		m_bUsingListTwo;
			HXBOOL		m_bIsQuitting;
			
			HXBOOL  		m_bDeferredTaskPending;

//#define _USE_OT_DEFER 1
#ifdef _USE_OT_DEFER
			long   		m_OTDeferredCookie;
    	static pascal void		DeferredTaskProc(void* param);
#else
			DeferredTask	m_DeferredTaskStruct;
    	static pascal void		DeferredTaskProc(long param);
#endif
    	
   	void AddToThePendingList(void* pNode);
   	void CleanupPendingLists();
   	void ProcessPendingCallbacks();
	virtual void ProcessCmd(OTEventCode code, OTResult result, void* cookie) {};
	
static OTResult GetFourByteOption(EndpointRef ep,
                           OTXTILevel level,
                           OTXTIName  name,
                           UInt32   *value);
                           
static OTResult SetFourByteOption(EndpointRef ep,
                           OTXTILevel level,
                           OTXTIName  name,
                           UInt32    value);
	
#ifdef _CARBON
		static OTClientContextPtr sOToutClientContext;
#endif
};

#endif // _OT_NET		

