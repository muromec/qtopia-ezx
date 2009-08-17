/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ot_net.cp,v 1.5 2005/03/14 20:28:50 bobclark Exp $
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

#include "hxcom.h"
#include "Mac_net.h"
#include "OT_net.h"
#include "OT_TCP.h"
#include "OT_UDP.h"
#include "hxmm.h"
#include "hxerrors.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "hxslist.h"

ULONG32 	OT_net::sMacAddress = 0L;

Boolean 	OT_net::sMacAddressValid = FALSE;
Boolean 	OT_net::sCheckedOT = FALSE;
Boolean 	OT_net::sHaveOT = FALSE;
Boolean 	OT_net::sMacAddressStrValid = FALSE;
char 		OT_net::sMacAddrStr[16];
#ifdef _CARBON
OTClientContextPtr OT_net::sOToutClientContext = NULL;
#endif

//#define _LOG_DATA	1

#if defined(_DEBUG) && defined (_LOG_DATA)
#define DEBUGSTR(x)	DebugStr(x)
#else
#define DEBUGSTR(x)
#endif

OT_net*
OT_net::new_socket(UINT16 type)
{
OT_net 		*c = NULL;

	switch(type)
	{
		case HX_TCP_SOCKET:
			c = new OT_TCP;
			break;
			
		case HX_UDP_SOCKET:
			c = new OT_UDP;
			break;
		
		
	}
	
	return(c);
}

// Creator should set the socket reference to a value indicating the socket is not open
OT_net::OT_net (void)

	: mActiveName (NULL)
	, mRef (0)	
	, mDataArrived (FALSE)
	, mDataFlowOn (FALSE)
	, mClosing (FALSE)
	, mOtherSideHasClosed (FALSE)
	, mWeHaveClosed (FALSE)
	, mRelease (FALSE)
	, mStartedReceivingData (FALSE)
	, mAsyncError(HXR_OK)
	, m_lRefCount(0)
    	, m_pPendingCallbackListOne(NULL)
    	, m_pPendingCallbackListTwo(NULL)
    	, m_bUsingListOne(FALSE)
    	, m_bUsingListTwo(FALSE)
    	, m_bDeferredTaskPending(FALSE)
    	, m_bIsQuitting(FALSE)
{
#ifdef _USE_OT_DEFER
    m_OTDeferredCookie = OTCreateDeferredTask(OT_net::DeferredTaskProc, this);
#else
    m_DeferredTaskStruct.dtReserved = 0;
    m_DeferredTaskStruct.dtFlags = 0;
#ifdef _CARBON
    m_DeferredTaskStruct.dtAddr = NewDeferredTaskUPP(OT_net::DeferredTaskProc);;
#else
    m_DeferredTaskStruct.dtAddr = NewDeferredTaskProc(OT_net::DeferredTaskProc);;
#endif
    m_DeferredTaskStruct.dtParam = (long) this; 	
    m_DeferredTaskStruct.qType = dtQType;
#endif    
    
}

// ~OT_net should close the socket if it is open
OT_net::~OT_net(void)
{
    m_bIsQuitting = TRUE;
    
    UINT32 timeout = TickCount() + 300L;
    while ( m_bDeferredTaskPending && timeout - TickCount() > 0 )
    {
	// sit-n-spin, awaiting completion of callbacks.
    }
    
#ifdef _USE_OT_DEFER
    if (m_OTDeferredCookie != 0)
    {
        OTDestroyDeferredTask(m_OTDeferredCookie);
        m_OTDeferredCookie = 0;
    }
#else
    if (m_DeferredTaskStruct.dtAddr != NULL)
    {
#ifdef _CARBON
	DisposeDeferredTaskUPP(m_DeferredTaskStruct.dtAddr);
#else
	DisposeRoutineDescriptor(m_DeferredTaskStruct.dtAddr);
#endif
	m_DeferredTaskStruct.dtAddr = NULL;
    }    
#endif
	
    CleanupPendingLists();
	
    HX_DELETE(m_pPendingCallbackListOne);
    HX_DELETE(m_pPendingCallbackListTwo);
	
	if(mRef) 				
		OTCloseProvider(mRef);
	
	mRef = 0;
	
	if (mActiveName) 
		delete [] mActiveName;
	mActiveName = nil;
}




void
OT_net::done (void)
{
	mLastError = HXR_OK;
}

#if 1
/*----------------------------------------------------------------------------
	OTWait 
	
	Wait for an asynchronous Open Transport stream call to complete.
	
	Exit:	function result = error code.
----------------------------------------------------------------------------*/
HX_RESULT
OT_net::OTWait (void)
{
Boolean cancel = FALSE;

	do {
		cancel = Mac_net::CheckForCancel();
	}
	while (!mComplete && !cancel);
	
	if(cancel)
		mResult = HXR_BLOCK_CANCELED;
	
	return mResult;
}
#else
/*----------------------------------------------------------------------------
	OTWait 
	
	Wait for an asynchronous Open Transport stream call to complete.
	
	Exit:	function result = error code.
----------------------------------------------------------------------------*/
HX_RESULT
OT_net::OTWait (void)
{
EventRecord macEvent;

	do {
		WaitNextEvent(nullEvent, &macEvent, 0L,NULL);
	}
	while (!mComplete);
	
	return mResult;
}
#endif
/*----------------------------------------------------------------------------
	OTSvcWait 
	
	Wait for an asynchronous Open Transport Internet services call to complete.
	
	Entry:	svcInfo = pointer to OTSvcInfo struct.
	
	Exit:	function result = error code.
----------------------------------------------------------------------------*/

HX_RESULT
OT_net::OTSvcWait (OTSvcInfo *svcInfo)
{
Boolean cancel = FALSE;
HX_RESULT theErr = HXR_OK;

	do 
	{
		cancel = Mac_net::CheckForCancel();
	}
	while (!svcInfo->complete && !cancel);

	if(cancel)
		theErr = HXR_BLOCK_CANCELED;
	else
		theErr = svcInfo->result;
	
	return theErr;
}

/*----------------------------------------------------------------------------
	NetHaveOT 
	
	Determine whether we have Open Transport.
	
	Exit:	function result = true if Open Transport and Open Transport/TCP
			are both installed.
----------------------------------------------------------------------------*/

Boolean 
OT_net::NetHaveOT (void)
{
	HX_RESULT theErr = HXR_OK;
	long result;
	
	if (!sCheckedOT) 
	{
		theErr = Gestalt(gestaltOpenTpt, &result);
		sHaveOT = theErr == HXR_OK && 
			(result & gestaltOpenTptPresentMask) != 0 &&
			(result & gestaltOpenTptTCPPresentMask) != 0;
		sCheckedOT = true;
	}
	
	return sHaveOT;
}

/*----------------------------------------------------------------------------
	MyOTInetSvcNotifyProc 
	
	Open Transport notifier proc for an Internet services provider.
	
	Entry:	svcIfno = pointer to MyOTInetSvcInfo struct.
			code = OT event code.
			result = OT result.
			cookie = OT cookie.
----------------------------------------------------------------------------*/

pascal void OT_net::SvcNotifyProc (

	void 		*stream, 
	OTEventCode code,
	OTResult 	result, 
	void 		*cookie )
	
{

	OTSvcInfo *svcInfo = (OTSvcInfo *) stream;
	
	switch (code) 
	{
		case T_OPENCOMPLETE:
		case T_DNRSTRINGTOADDRCOMPLETE:
		case T_DNRADDRTONAMECOMPLETE:
			svcInfo->complete = true;
			svcInfo->result = result; 
			svcInfo->cookie = cookie;
			break;
	}
}

/*----------------------------------------------------------------------------
	OpenInetServices 
	
	Open an Internet services provider.
	
	Entry:	svcInfo = pointer to OTSvcInfo struct for this
				provider.
	
	Exit:	function result = error code.
----------------------------------------------------------------------------*/

HX_RESULT
 OT_net::OpenInetServices (OTSvcInfo *svcInfo)
{
	HX_RESULT theErr = HXR_OK;

	svcInfo->complete = false;
#ifdef _CARBON
	theErr = OTAsyncOpenInternetServicesInContext(kDefaultInternetServicesPath, 0,
					(OTNotifyUPP)SvcNotifyProc, svcInfo, NULL);
#else
	theErr = OTAsyncOpenInternetServices(kDefaultInternetServicesPath, 0, 
						SvcNotifyProc, svcInfo);
#endif

	if(!theErr) theErr = OTSvcWait(svcInfo);
	if(!theErr) svcInfo->ref = (InetSvcRef) svcInfo->cookie;
	
	return theErr;
}


/*----------------------------------------------------------------------------
	NetGetMyAddr 
	
	Get this Mac's IP address.
	
	Exit:	function result = error code.
			*addr = the IP address of this Mac.
			
	With Open Transport, if the Mac has more than one IP interface, the
	IP address of the default interface is returned.
----------------------------------------------------------------------------*/

HX_RESULT OT_net::NetGetMyAddr (unsigned long *addr)
{
HX_RESULT theErr = HXR_OK;
InetInterfaceInfo ifaceInfo;
	
	if (!sMacAddressValid) 
	{
		theErr = OTInetGetInterfaceInfo(&ifaceInfo, kDefaultInetInterface);
		if (theErr != HXR_OK) return (theErr);
			sMacAddress = ifaceInfo.fAddress;
		
		sMacAddressValid = true;
	}
	
	*addr = sMacAddress;
	return HXR_OK;
}

HX_RESULT OT_net::NetGetIfaceInfo(InetInterfaceInfo* ifaceInfo)
{
 	return (OTInetGetInterfaceInfo(ifaceInfo, kDefaultInetInterface));
}

HX_RESULT	
OT_net::blocking(void)
{

	return(HXR_OK);
}

HX_RESULT	
OT_net::nonblocking(void)
{

	return(HXR_OK);
}
	

/*----------------------------------------------------------------------------
	NetGetMyAddrStr 
	
	Get this Mac's IP address as a dotted-decimal string

	Exit:	function result = error code.
			name = this Mac's IP address, as a C-format string.
				You must allocate at least 16 bytes for this string.
				The returned string has max length 15.
----------------------------------------------------------------------------*/
	
HX_RESULT OT_net::NetGetMyAddrStr (char *addrStr)
{
	unsigned long addr;
	HX_RESULT theErr = HXR_OK;
	
	if (!sMacAddressStrValid) 
	{
		theErr = NetGetMyAddr(&addr);
		if (theErr != HXR_OK) return (theErr);
		
		sprintf(sMacAddrStr, "%ld.%ld.%ld.%ld",
				(addr >> 24) & 0xff,
				(addr >> 16) & 0xff,
				(addr >> 8) & 0xff,
				 addr & 0xff);
				
		sMacAddressStrValid = TRUE;
	}
	strcpy(addrStr, sMacAddrStr);
	return HXR_OK;
}

/*----------------------------------------------------------------------------
	NetNameToAddr 
	
	Translate a domain name to an IP address.
	
	Entry: 	name = C-format domain name string, optionally followed by a
				comma, space, or colon and then the port number.
			defaultPort = default port number.
	
	Exit:	function result = error code.
			*addr = IP address.
			*port = port number.
----------------------------------------------------------------------------*/

HX_RESULT OT_net::NetNameToAddr (

	char *name, 
	unsigned short defaultPort, 
	unsigned long *addr, 
	unsigned short *port)
	
{
	HX_RESULT theErr = HXR_OK;

	InetHostInfo hInfoOT;
	char domainName[256];
	char *p, *q;
	OTSvcInfo svcInfo;
	
	
	p = name;
	q = domainName;
	while (*p != 0 && *p != ',' && *p != ' ' && *p != ':') *q++ = *p++;
	*q = 0;
	q = p;
	while (*q == ' ') q++;
	if (*q == 0) {
		*port = defaultPort;
	} else {
		p++;
		if (!isdigit(*p)) return HXR_BIND;
		q = p+1;
		while (isdigit(*q)) q++;
		while (*q == ' ') q++;
		if (*q != 0) return HXR_BIND;
		*port = atoi(p);
	}
	
	if(conn::is_cached(domainName,addr))
		return HXR_OK;
	
	theErr = OTInetStringToHost(domainName, addr);
	if (theErr != HXR_OK) {
		theErr = OpenInetServices(&svcInfo);
		if (theErr == kEINVALErr) return HXR_BIND;
		if (theErr != HXR_OK) return (theErr);
		svcInfo.complete = false;
		theErr = OTInetStringToAddress(svcInfo.ref, domainName, &hInfoOT);
		if (theErr == HXR_OK) theErr = OTSvcWait(&svcInfo);
		OTCloseProvider(svcInfo.ref);
		if (theErr != HXR_OK) {
			if (theErr == kOTNoDataErr || theErr == kOTBadNameErr) theErr = HXR_BIND;
			return (theErr);
		}
		OTCloseProvider(svcInfo.ref);
	}
	
	conn::add_to_cache(domainName, *addr);

	
	if (mActiveName) 
	{
		delete [] mActiveName;
		mActiveName = NULL;
	}
	mActiveName = (Ptr) new char[ (::strlen (domainName) + 1)];
	if (HXR_OK != (theErr = ::MemError ())) return (theErr);
	::strcpy (mActiveName, domainName);
	
	return HXR_OK;
}



/*----------------------------------------------------------------------------
	NetAddrToName 
	
	Translate an IP address to a domain name.
	
	Entry:	addr = IP address.
	
	Exit:	function result = error code.
			name = domain name, as a C-format string.
----------------------------------------------------------------------------*/

HX_RESULT OT_net::NetAddrToName (unsigned long addr, InetDomainName name)
{
	HX_RESULT theErr = HXR_OK;
	OTSvcInfo svcInfo;
	
		theErr = OpenInetServices(&svcInfo);
		if (theErr != HXR_OK) return (theErr);
		svcInfo.complete = false;
		theErr = OTInetAddressToName(svcInfo.ref, addr, name);
		if (theErr == HXR_OK) theErr = OTSvcWait(&svcInfo);
		OTCloseProvider(svcInfo.ref);
		if (theErr != HXR_OK) {
			if (theErr == kOTNoDataErr || theErr == kOTBadNameErr) theErr = HXR_BIND;
			return (theErr);
		}
		return HXR_OK;	
}


// init_drivers() should do any network driver initialization here
// params is a pointer to a platform specfic defined struct that 
// contains an required initialization data

HX_RESULT
OT_net::init_drivers(void *params)
{
	return(NetInit());
}

HX_RESULT OT_net::NetInit (void)
{
HX_RESULT theErr = HXR_OK;

	if(NetHaveOT())
	{
	
#ifdef _CARBON
		theErr = InitOpenTransportInContext(kInitOTForApplicationMask, &sOToutClientContext);
		
		// in the embedded player case, the above call may fail
		// so try again with kInitOTForExtensionMask flag
		// sOToutClientContext has to be provided when not calling from an application
		if (theErr != HXR_OK)
		{
			theErr = InitOpenTransportInContext(kInitOTForExtensionMask, &sOToutClientContext);
		}
		HX_ASSERT( theErr == HXR_OK );
		if (theErr != HXR_OK) return HXR_OK;
#else
		theErr = InitOpenTransport();
#endif
		if (theErr != HXR_OK) return HX_OPEN_DRIVER_ERROR;
	}
	else
		theErr = -1;
	
	return(theErr);
}		

// close_drivers() should close any network drivers used by the program
// NOTE: The program MUST not make any other calls to the network drivers
// until init_drivers() has been called

HX_RESULT
OT_net::close_drivers(void *params)
{	
	if(sHaveOT)
	{
#ifdef _CARBON
		CloseOpenTransportInContext(sOToutClientContext);
#else
		CloseOpenTransport();
#endif
	}
	
	return(HXR_OK);
}

	
HX_RESULT
OT_net::host_to_ip_str(char *host, char *ip, UINT32 ulBufLen)
{
	HX_RESULT 	theErr = HXR_OK;
	ULONG32 	addr;
	UINT16		port;
	
	theErr = lookup_host(host,&addr, &port);

	if(!theErr) OTInetHostToString(addr,ip);
	
	if(theErr) theErr = HXR_BIND;
	
	return(theErr);
}

ULONG32	OT_net::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG32 OT_net::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


HX_RESULT
OT_net::lookup_host (

	char 		*name, 
	ULONG32 	*addr, 
	UINT16 		*port,
	UINT16 		defaultPort)
	
{
	HX_RESULT theErr = HXR_OK;

	InetHostInfo hInfoOT;
	char domainName[256];
	char *p, *q;
	OTSvcInfo svcInfo;
	
	p = name;
	q = domainName;

	while (*p != 0 && *p != ',' && *p != ' ' && *p != ':') *q++ = *p++;

	*q = 0;
	q = p;

	while (*q == ' ') q++;

	if (*q == 0) 
		*port = defaultPort;
	else 
	{
		p++;
		if (!isdigit(*p)) return HXR_BIND;
		q = p+1;
		while (isdigit(*q)) q++;
		while (*q == ' ') q++;
		if (*q != 0) return HXR_BIND;
		*port = atoi(p);
	}
	
	// check if DNS has been cached
	if(conn::is_cached(domainName,addr))
		return HXR_OK;
	
	theErr = OTInetStringToHost(domainName, addr);
	if (theErr != HXR_OK) 
	{
		theErr = OpenInetServices(&svcInfo);
		if (theErr == kEINVALErr) return HXR_BIND;
		if (theErr != HXR_OK) return (theErr);
		svcInfo.complete = false;
		theErr = OTInetStringToAddress(svcInfo.ref, domainName, &hInfoOT);
		if (theErr == HXR_OK) theErr = OTSvcWait(&svcInfo);
		OTCloseProvider(svcInfo.ref);
		if (theErr != HXR_OK) 
		{
			if (theErr == kOTNoDataErr || theErr == kOTBadNameErr) theErr = HXR_BIND;
			return (theErr);
		}
		*addr = hInfoOT.addrs[0];
	}
	
	conn::add_to_cache(domainName, *addr);
	
	return HXR_OK;
}


HX_RESULT OT_net::dns_find_ip_addr(const char * host, UINT16 blocking)
{
	if(!host)                 
	{
		mLastError = HX_INVALID_HOST;
		return mLastError;
	}
//XXXCWB
//	if(get_sock() < 0)                 
//	{
//		mLastError = HX_NET_SOCKET_INVALID;
//		return mLastError;
//	}

	if (conn::is_cached((char *) host, &mHostIPAddr))
	{
		mHostIPValid = TRUE;
		mDNSDone	= TRUE;
		if (mCallBack)	 
		{
		    mCallBack->Func(DNS_NOTIFICATION, TRUE);
		}
		return( mLastError = HXR_OK);
	}

        char* pTemp = strrchr(host, '.');
        if (pTemp && atoi(pTemp + 1))
        {   /* IP address. */

		struct in_addr addr;  
		mHostIPValid = FALSE;
		mHostIPAddr = 0;
		mDNSDone	= TRUE;

		::OTInetStringToHost((char*)host, &addr.s_addr); //!!!!!
		
		if ((UINT)addr.s_addr == (UINT)-1) 
		{
			mLastError = HX_INVALID_HOST;
			if (mCallBack)	 
			{
			    mCallBack->Func(DNS_NOTIFICATION, FALSE);
			}
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

		InetHostInfo		ihost;
		OTSvcInfo svcInfo;
		OSErr theErr = OpenInetServices(&svcInfo);
		if (theErr == kEINVALErr) return HXR_BIND;
		if (theErr != HXR_OK) return (theErr);
		svcInfo.complete = false;

		::OTInetStringToAddress(svcInfo.ref,(char*)host,&ihost);
		if (theErr == HXR_OK) theErr = OTSvcWait(&svcInfo);
		OTCloseProvider(svcInfo.ref);
		
		if (!ihost.addrs ) 
		{
			mLastError = HX_INVALID_HOST;

			if (mCallBack)	 
			{
			    mCallBack->Func(DNS_NOTIFICATION, FALSE);
			}

			return mLastError;
		}

		memcpy(&addr, ihost.addrs, sizeof(struct in_addr));

		mHostIPValid = TRUE;
		mHostIPAddr = *(ULONG32 *) &addr;
		conn::add_to_cache((char *) host, mHostIPAddr);

		if (mCallBack)	 
		{
		    mCallBack->Func(DNS_NOTIFICATION, TRUE);
		}

		return( mLastError = HXR_OK);
	}
	else	// non-blocking
	{	
		mDNSDone     = TRUE;
		mHostIPValid = FALSE;
		mHostIPAddr  = 0;
		
		HX_RESULT theErr;
		MyOTInfo myOTInfo;
		
		::memset(&myOTInfo, 0, sizeof(myOTInfo));
		
		myOTInfo.hostName = host;
		myOTInfo.complete = false;
		
#ifdef _CARBON
		theErr = OTAsyncOpenInternetServicesInContext(kDefaultInternetServicesPath, 0,
					::NewOTNotifyUPP(MyOTNotifyProc), &myOTInfo, NULL);
#else
		theErr = OTAsyncOpenInternetServices(kDefaultInternetServicesPath, 0,
							MyOTNotifyProc, &myOTInfo);
#endif
		if (theErr != HXR_OK)
		{
			return( mLastError = HXR_BIND);
		}
		
		// MyOTNotifyProc() handles asynch. functionality of this routine
		
		Boolean cancel = false;
		do 
		{
			cancel = Mac_net::CheckForCancel();
		}
		while (!myOTInfo.complete && !cancel);

		// Save the IP address
		Boolean bFound;
		if (myOTInfo.hostInfo.addrs && *myOTInfo.hostInfo.addrs)
		{
			bFound = true;
			mLastError = HXR_OK;
			 
			struct in_addr addr;  
			memcpy(&addr, myOTInfo.hostInfo.addrs, sizeof(struct in_addr));

			mHostIPValid = TRUE;
			mHostIPAddr = *(ULONG32 *) &addr;
			conn::add_to_cache((char *) host, mHostIPAddr);
		}
		else
		{
			bFound = false;
			mLastError = HX_INVALID_HOST;
		}

		::OTRemoveNotifier(myOTInfo.inetSvcRef);
		::OTCloseProvider (myOTInfo.inetSvcRef);
			
		if (mCallBack)	 
		{
		    mCallBack->Func(DNS_NOTIFICATION, bFound);
		}

		return(mLastError);
	}
	return FALSE;
}

pascal void OT_net::MyOTNotifyProc
(
	void* 		contextPtr, 
	OTEventCode 	code,
	OTResult 	result, 
	void* 		cookie
)
{
	MyOTInfo* myOTInfoPtr = (MyOTInfo *) contextPtr;
	
	switch (code) 
	{
		case T_OPENCOMPLETE:
			myOTInfoPtr->inetSvcRef = (InetSvcRef)cookie;
			::OTInetStringToAddress(myOTInfoPtr->inetSvcRef, (char*)myOTInfoPtr->hostName, &myOTInfoPtr->hostInfo);
			break;
			
		case T_DNRSTRINGTOADDRCOMPLETE:
		case T_DNRADDRTONAMECOMPLETE:
			myOTInfoPtr->complete = true;
			break;
	}
}


HXBOOL OT_net::dns_ip_addr_found(HXBOOL * valid, ULONG32 *addr)
{
	if (mDNSDone)
	{
		*valid = mHostIPValid;
		*addr  = mHostIPAddr;

		return TRUE;
	}
	else
		return FALSE;

	return FALSE;
}


/***********Common code for OT_UDP/OT_TCP*************************/

pascal void OT_net::UDPTCPNotifyProc (

	 void *stream, 
	 OTEventCode code,
	 OTResult result, 
	 void *cookie )
	 
{
	HXMM_INTERRUPTON();

	OT_net* s =  (OT_net*) stream;

#ifdef _USE_OT_DEFER
        if (s->m_OTDeferredCookie != 0)
#else
	if (s->m_DeferredTaskStruct.dtAddr != NULL)
#endif
	{
	    OTCallbackInfo* pOTCallbackInfo = new OTCallbackInfo(code, result, cookie); 
	    
	    s->AddToThePendingList((void*) pOTCallbackInfo);
	    
	    if (!s->m_bDeferredTaskPending)
	    {
	        if ( !s->m_bIsQuitting )
	        {
		        s->m_bDeferredTaskPending = TRUE;
#ifdef _USE_OT_DEFER
			OTScheduleDeferredTask(s->m_OTDeferredCookie);
#else
		        DTInstall(&s->m_DeferredTaskStruct);
#endif
	        }
	    }
	}
    
    HXMM_INTERRUPTOFF();
}

#ifdef _USE_OT_DEFER
pascal void OT_net::DeferredTaskProc(void* inParam)
#else
pascal void OT_net::DeferredTaskProc(long inParam)
#endif
{
    HXMM_INTERRUPTON();

    OT_net* s =  (OT_net*) inParam;
    if(s)
    {
	if ( !s->m_bIsQuitting )
	{
	    s->ProcessPendingCallbacks();
	}
        s->m_bDeferredTaskPending = FALSE;
    }
    
    HXMM_INTERRUPTOFF();
}

void OT_net::AddToThePendingList(void* pNode)
{
    /* Atleast one of the list MUST be free to operate on */
    HX_ASSERT(!m_bUsingListOne || !m_bUsingListTwo);
		
    if (!m_bUsingListOne)
    {
        if (!m_pPendingCallbackListOne)
        {
            m_pPendingCallbackListOne = new CHXSimpleList;
        }
        
        m_pPendingCallbackListOne->AddTail(pNode);
    }
    else if (!m_bUsingListTwo)
    {
        if (!m_pPendingCallbackListTwo)
        {
            m_pPendingCallbackListTwo = new CHXSimpleList;
        }
        
        m_pPendingCallbackListTwo->AddTail(pNode);
    }    
}		


void OT_net::ProcessPendingCallbacks()
{
start:
    m_bUsingListOne = TRUE;

    while (m_pPendingCallbackListOne && 
          !m_pPendingCallbackListOne->IsEmpty())
    {
	OTCallbackInfo*	cmd = (OTCallbackInfo*) m_pPendingCallbackListOne->RemoveHead();
		
	/* Send time sync for ONLY the last pending audio callback */
	ProcessCmd(cmd->m_Code, cmd->m_Result, cmd->m_Cookie);
				
     	delete cmd;
    }
	
    m_bUsingListOne = FALSE;

    m_bUsingListTwo = TRUE;

    while (m_pPendingCallbackListTwo && 
          !m_pPendingCallbackListTwo->IsEmpty())
    {
	OTCallbackInfo*	cmd = (OTCallbackInfo*) m_pPendingCallbackListTwo->RemoveHead();
		
	/* Send time sync for ONLY the last pending audio callback */
	ProcessCmd(cmd->m_Code, cmd->m_Result, cmd->m_Cookie);
		
    	delete cmd;
    }
	
    m_bUsingListTwo = FALSE;
    
    /* Do we still have more pending callbacks to process? */
    if ((m_pPendingCallbackListOne && m_pPendingCallbackListOne->GetCount() > 0) ||
        (m_pPendingCallbackListTwo && m_pPendingCallbackListTwo->GetCount() > 0))
    {
        goto start;
    }    
}        

void OT_net::CleanupPendingLists()
{
    m_bUsingListOne = TRUE;
    while(m_pPendingCallbackListOne && !m_pPendingCallbackListOne->IsEmpty())
    {
        SndCommand*	cmd = (SndCommand*)m_pPendingCallbackListOne->RemoveHead();	
        delete cmd;		
    }
	
    m_bUsingListOne = FALSE;
	
    m_bUsingListTwo = TRUE;
    while(m_pPendingCallbackListTwo && !m_pPendingCallbackListTwo->IsEmpty())
    {
        SndCommand*	cmd = (SndCommand*)m_pPendingCallbackListTwo->RemoveHead();	
        delete cmd;		
    }
	
    m_bUsingListTwo = FALSE;
}

OTResult OT_net::GetFourByteOption(EndpointRef ep,
                           OTXTILevel level,
                           OTXTIName  name,
                           UInt32   *value)
{
   OTResult err;
   TOption  option;
   TOptMgmt request;
   TOptMgmt result;
   
   /* Set up the option buffer */
   option.len  = kOTFourByteOptionSize;
   option.level= level;
   option.name = name;
   option.status = 0;
   option.value[0] = 0;// Ignored because we're getting the value.

   /* Set up the request parameter for OTOptionManagement to point
    to the option buffer we just filled out */

   request.opt.buf= (UInt8 *) &option;
   request.opt.len= sizeof(option);
   request.flags= T_CURRENT;

   /* Set up the reply parameter for OTOptionManagement. */
   result.opt.buf  = (UInt8 *) &option;
   result.opt.maxlen = sizeof(option);
   
   err = OTOptionManagement(ep, &request, &result);

   if (err == noErr) {
      switch (option.status) 
      {
         case T_SUCCESS:
         case T_READONLY:
            *value = option.value[0];
            break;
         default:
            err = option.status;
            break;
      }
   }
            
   return (err);
}

OTResult OT_net::SetFourByteOption(EndpointRef ep,
                           OTXTILevel level,
                           OTXTIName  name,
                           UInt32     value)
{
   OTResult err;
   TOption  option;
   TOptMgmt request;
   TOptMgmt result;
   
   /* Set up the option buffer */
   option.len  = kOTFourByteOptionSize;
   option.level= level;
   option.name = name;
   option.status = 0;
   option.value[0] = value; // set the value here

   /* Set up the request parameter for OTOptionManagement to point
    to the option buffer we just filled out */

   request.opt.buf= (UInt8 *) &option;
   request.opt.len= sizeof(option);
   request.flags= T_NEGOTIATE;

   /* Set up the reply parameter for OTOptionManagement. */
   result.opt.buf  = (UInt8 *) &option;
   result.opt.maxlen = sizeof(option);
   
   err = OTOptionManagement(ep, &request, &result);

   if (err == noErr) {
      switch (option.status) 
      {
         case T_SUCCESS:
         case T_READONLY:
            value = option.value[0];
            break;
         default:
            err = option.status;
            break;
      }
   }
            
   return (err);
}


