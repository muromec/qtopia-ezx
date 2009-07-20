/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ot_tcp.cp,v 1.6 2007/07/06 21:57:58 jfinnecy Exp $
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

#include "hxcom.h"
#include "OT_TCP.h"

#include "MWDebug.h"
#include "hxerrors.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "hxmm.h"

//#define _LOG_DATA	1
//#define USE_DEFERRED_IMPL 1

#if defined(_DEBUG) && defined (_LOG_DATA)
#define DEBUGSTR(x)	DebugStr(x)
#else
#define DEBUGSTR(x)
#endif

//#include "dcon.h"

OT_TCP::OT_TCP (void)
	
	: mDNSOpen (FALSE)
	, mSocketOpen (FALSE)
	, mState (TCP_STATE_CLOSED)
	, mDNSRef (0)
	, mRemoteHostName (0)
	, mNewConnection(NULL)
	, mDataReady(FALSE)

	{ /* begin OT_TCP */
		//dprintf("this:%X OT_TCP::OT_TCP\n", this);
	} /* end OT_TCP */
	
OT_TCP::~OT_TCP (void)
	
	{ /* begin ~OT_TCP */
		//dprintf("this:%X OT_TCP::~OT_TCP\n", this);
	
		Cleanup(TRUE); 

		if(mRemoteHostName)
		{
			delete mRemoteHostName;
			mRemoteHostName = NULL;
		}


	} /* end ~OT_TCP */
	

void
OT_TCP::done(void)
{
	//dprintf("this:%X OT_TCP::done\n", this);
	Cleanup(TRUE);
}




/*----------------------------------------------------------------------------
	Cleanup 
	
	Cleanup a TCP stream.
	
	Exit:	function result = error code.
	
	Any active connection is also aborted, if necessary, before releasing 
	the stream.
	
----------------------------------------------------------------------------*/

OSErr OT_TCP::Cleanup (Boolean orderly /* = FALSE */ )
{
	//dprintf("this:%X OT_TCP::Cleanup orderly:%s\n", this, (orderly? "TRUE" : "FALSE"));
	OSErr theErr = HXR_OK;
	Boolean abort;

	mState = TCP_STATE_CLOSED;
	
	close_resolver();

	if(mRef)
	{
		
		abort = (!mOtherSideHasClosed || !mWeHaveClosed) && mConnectionOpen;
		
		if (abort) 
		{
			mComplete = FALSE;
			if (orderly && mStartedReceivingData)
			{
				mComplete = FALSE;
				theErr = ::OTSndOrderlyDisconnect(mRef);
				
				EventRecord macEvent;
				long ticks = TickCount();
				
				do 
				{
#ifndef THREADS_SUPPORTED
					// xxxbobclark when threading networking is
					// on, this code may be executing on a non-main-
					// app thread. I don't even know what WNE would
					// do in that scenario, but that it'd be bad.
					WaitNextEvent(nullEvent, &macEvent, 6,NULL);
#endif
				}
				while (!mComplete && ((TickCount() - ticks) < 120)); 
			}

			else
				theErr = ::OTSndDisconnect(mRef, nil);
		}	

		::OTRemoveNotifier(mRef);		// DS 12.4.95
		theErr = ::OTCloseProvider(mRef);
	}
	
	if ( mNewConnection )
	{
		mNewConnection->done();
		HX_RELEASE(mNewConnection);
	}
	
	mRef = 0;
	
	return theErr;
}

 HX_RESULT
 OT_TCP::connect (
 
	const char	*host, 
	UINT16 		port,
	UINT16 		blocking,
	ULONG32		ulPlatform )
		
{
	//dprintf("this:%X OT_TCP::connect - host:%s port:%u\n", this, host, port);
	HX_RESULT theErr = HXR_OK;

	// check if socket is in a valid state
	if(mState != TCP_STATE_CLOSED)
		return HXR_SOCKET_CREATE;  // should be PN_SOCKET_STATE_ERROR
	
	if(mRemoteHostName)
	{
		 delete mRemoteHostName; 
		 mRemoteHostName = NULL;
	}
	
	// allocate a buffer to hold the host name
	mRemoteHostName = new char[::strlen(host) + 1];

	if(mRemoteHostName == NULL)
		theErr = HXR_OUTOFMEMORY;

	if(!theErr)
	{
		mConnectionOpen = FALSE;
		mRemotePort = port;
		mDataFlowOn = FALSE;
		mDataArrived = FALSE;
		mAsyncError = HXR_OK;
		
		// save a copy of the host name 
		::strcpy(mRemoteHostName,host);

		// check if host name is already in IP format or has been cached
		if((::OTInetStringToHost(mRemoteHostName, &mRemoteHost) == HXR_OK) || 
		  (conn::is_cached(mRemoteHostName,&mRemoteHost)))
		{
			// open the socket, bind and connect to remote host
			mState = TCP_STATE_OPEN_SOCKET;
			theErr = open_socket();
		}
		else // DNR is required on host name
		{
			// do DNR,open the socket, bind and connect to remote host
			theErr = open_resolver();
		}
	}
	
	if(!theErr && blocking)
	{
		// wait for mConnectionOpen == TRUE or cancel
	
	}
	
	return theErr;
}

	

 HX_RESULT
 OT_TCP::init (
 	UINT32	local_addr,
	UINT16 	port,
	UINT16 	blocking)
		
{
	//dprintf("this:%X OT_TCP::init\n", this);
	return (HXR_INVALID_OPERATION);
}




/*----------------------------------------------------------------------------
	write 
	
	Send data on a stream.
	
	Entry:	data = pointer to data to send.
			len = length of data to send.
	
	Exit:	function result = error code.
----------------------------------------------------------------------------*/

HX_RESULT 
OT_TCP::write (
	
	void *data, 
	UINT16 *len)
	
{
	//dprintf("this:%X OT_TCP::write\n", this);
	HX_RESULT theErr = HXR_OK;

	mLastError=HXR_OK;

	if(mAsyncError)
	{
		theErr = mAsyncError;
		mAsyncError = HXR_OK;
		return theErr;
	}

	OTResult 		result;
	unsigned short 	length, count;
	
	count = 0;
	
	if (mOtherSideHasClosed) //cz 
		theErr = HXR_SERVER_DISCONNECTED;
	else if (!mOtherSideHasClosed && !mConnectionOpen) 
		theErr = HXR_WOULD_BLOCK;
	
	else
	{
#if 1
		if(mDataFlowOn)
		{
//			DebugStr("\pDataFlowOn");
			*len = 0;
			 return(HXR_WOULD_BLOCK);
		}
#endif
		
		length = *len;
		while (length > 0 && !theErr) 
		{
			result = ::OTSnd(mRef, data, length, 0);
			if (result >= 0) 
			{
				count += result;
				length -= result;
			} 
			
			else
			{
#if 0
				Str255 s;
				NumToString(result,s);
				DebugStr(s);
#endif				
				switch(result)
				{
					case kOTFlowErr:
						mWriteReady = FALSE;
						mDataFlowOn = TRUE;
						theErr = HXR_WOULD_BLOCK;
						break;
						
					case kENOMEMErr:
					case kEBUSYErr:
					case kOTOutStateErr:
					case kEWOULDBLOCKErr:
					case kOTStateChangeErr:
					case kOTLookErr:
						mLastError = HXR_WOULD_BLOCK;
						theErr = HXR_WOULD_BLOCK;
						break;
						
					default:
#if 0
						Str255 s;
						DebugStr("\pOT_TCP::read");
						NumToString(result,s);
						DebugStr(s);
#endif
						mLastError = HXR_SERVER_DISCONNECTED;
						theErr = HXR_SERVER_DISCONNECTED; 
						break;
				}
				
				break; // break out of the while loop
			}
		}
		
	}
	*len = count;		// return actual number of bytes written
	
	return theErr;
}

/*----------------------------------------------------------------------------
	read 
	
	read data on a stream.
	
	Entry:	data = pointer to data buffer.
			*len = length of data buffer.
	
	Exit:	function result = error code.
			*len = number of bytes received.
----------------------------------------------------------------------------*/

HX_RESULT
OT_TCP::read (void *data, UINT16 *len)
{
	//dprintf("this:%X OT_TCP::read\n", this);
	//dprintf("mConnectionOpen: %s\n", (mConnectionOpen ? "TRUE" : "FALSE") );
	//dprintf("mOtherSideHadClosed: %s\n", (mOtherSideHasClosed ? "TRUE" : "FALSE")); 
	//dprintf("mDataArrived: %s\n", (mDataArrived ? "TRUE" : "FALSE"));

	HX_RESULT theErr = HXR_OK;
	OTResult result;

	mLastError=HXR_OK;

	if(mAsyncError)
	{
		theErr = mAsyncError;
		mAsyncError = HXR_OK;
		return theErr;
	}
	
	if (mOtherSideHasClosed)  
		theErr = HXR_SERVER_DISCONNECTED;
	else if (!mOtherSideHasClosed && !mConnectionOpen) 
		theErr = HXR_WOULD_BLOCK;
	
	else
	{
		if(!mDataArrived)
		{
			*len = 0;
			mLastError = HXR_WOULD_BLOCK;
			return HXR_WOULD_BLOCK;
		}
		
		result = ::OTRcv(mRef, data, *len, nil);
		
		if (result >= 0) 
		{
			//dprintf("data read\n");
			*len = result;
			theErr = HXR_OK;
		} 
		else
		{
			//dprintf("read error\n");
			switch(result)
			{
				/* take out this case after HXTCPSocket::TCPSchedulerCallbackProc
				   gets implemented - we're not getting data fast enough now XXXZach */
				case kOTNoDataErr:  
					*len = 0;
					theErr = HXR_OK;
					break;
				case kEBUSYErr:
				case kOTOutStateErr:
				case kEWOULDBLOCKErr:
				case kOTStateChangeErr:
				case kOTLookErr:
					*len = 0;
					mLastError = HXR_WOULD_BLOCK;
					theErr = HXR_WOULD_BLOCK;
					break;
					
				default:
#if 0
					Str255 s;
					DebugStr("\pOT_TCP::read");
					NumToString(result,s);
					DebugStr(s);
#endif
					*len = 0;
					mLastError = HXR_SERVER_DISCONNECTED;
					theErr = HXR_SERVER_DISCONNECTED; //cz
					break;
			}
		}
	}
		
	return theErr;
}

HX_RESULT	
OT_TCP::readfrom (REF(IHXBuffer*)   pBuffer,
				  REF(UINT32)	    ulAddress,
				  REF(UINT16)	    ulPort)
{
	//dprintf("this:%X OT_TCP::readfrom\n", this);
	pBuffer = NULL;
	ulAddress = 0;
	ulPort = 0;

	return HXR_NOTIMPL;
}

HX_RESULT OT_TCP::listen(ULONG32 ulLocalAddr,
			 UINT16 port,
			 UINT16 backlog, 
			 UINT16 blocking, 
			 ULONG32 ulPlatform)
{
	//dprintf("this:%X OT_TCP::listen address:%X port:%u\n", this, ulLocalAddr, port);
	// check if socket is in a valid state
	if(mState != TCP_STATE_CLOSED)
		return HXR_UNEXPECTED;  // should be PN_SOCKET_STATE_ERROR

	OSErr theErr = HXR_OK;
	InetAddress reqAd,retAd;
	TBind req,ret;

	// Open TCP endpoint
	mState = TCP_STATE_OPEN_LISTEN;
	mComplete = false;
	// Note:  We are using the UDPTCPNotifyProc... Therefore we do
	// not get a Proccess command call at interupt time.  Normal
	// operation somehow gets away with it...
#ifdef _CARBON
	theErr = ::OTAsyncOpenEndpointInContext(OTCreateConfiguration(kTCPName), 0,NULL,
			(OTNotifyUPP)UDPTCPNotifyProc, this, NULL);
#else
	theErr = ::OTAsyncOpenEndpoint(OTCreateConfiguration(kTCPName), 
			0,NULL, UDPTCPNotifyProc, this);
#endif

	if ( theErr )
	{
		mLastError = theErr;
		theErr = HXR_SOCKET_CREATE;
	}
	// wait for open completion
	if(!theErr)
	{
		theErr = OTWait();
		if(theErr == kOTNoDataErr) theErr = HXR_OK;
		
		if(theErr || mCode != T_OPENCOMPLETE) 
		{
			theErr = theErr ? theErr : HXR_SOCKET_CREATE;
			mLastError = theErr;
			theErr = HXR_SOCKET_CREATE;
		}
	}

	if ( !theErr )
	{
		// get Inet address of port
		OTInitInetAddress(&reqAd, port, (InetHost) ulLocalAddr);

		// bind tcp to current address and requested port
		req.addr.maxlen = sizeof(struct InetAddress);
		req.addr.len = sizeof(struct InetAddress);
		req.addr.buf = (unsigned char *) &reqAd;
		req.qlen = 1;

		ret.addr.maxlen = sizeof(struct InetAddress);
		ret.addr.len = sizeof(struct InetAddress);
		ret.addr.buf = (unsigned char *) &retAd;

		// clear completion flag
		mComplete = false;

		mState = TCP_STATE_BIND_LISTEN;
		// bind provider to return structure
		theErr = OTBind(mRef, &req, &ret);
	}
	if(theErr)
	{
		mLastError = theErr;
		theErr = HXR_BIND;
	}
	
	// wait for bind completion
	if(!theErr)
	{
		theErr = OTWait();
		if(theErr == kOTNoDataErr) theErr = HXR_OK;
		
		if(theErr || mCode != T_BINDCOMPLETE) 
		{
			theErr = theErr ? theErr : HXR_BIND;
			mLastError = theErr;
			theErr = HXR_BIND;
		}
	}

	// get local port
	if(!theErr)
	{
		mConnectionOpen = FALSE;
		mRemotePort = port;
		mDataFlowOn = FALSE;
		mDataArrived = FALSE;
		mAsyncError = HXR_OK;
		
		mState = TCP_STATE_LISTEN;
	}
	if ( !theErr )
	{
		// check to see if we are not being called again...
		// calling done() on mNewConnection should be fine, because
		// it should be released after a connection has been accepted. 
		if ( mNewConnection )
		{
			mNewConnection->done();
			HX_RELEASE(mNewConnection);
		}
		mNewConnection = conn::actual_new_socket(HX_TCP_SOCKET);
		mNewConnection->AddRef();
		// we can wait here, because we should not be called
		// from deffered task time.
		theErr = mNewConnection->SetupEndpoint(TRUE);
	}
	return theErr;
}

HX_RESULT OT_TCP::resolve_address (void)
{
	//dprintf("this:%X OT_TCP::resulve_address\n", this);
	mState = TCP_STATE_RESOLVE_DNS;
	return ::OTInetStringToAddress(mDNSRef, mRemoteHostName, &mHInfoOT);
}

HX_RESULT
OT_TCP::open_resolver(void)
{
	//dprintf("this:%X OT_TCP::open_resolver\n", this);
	mState = TCP_STATE_OPEN_RESOLVER;
#ifdef USE_DEFERRED_IMPL	
	HX_RESULT theErr = ::OTAsyncOpenInternetServices(kDefaultInternetServicesPath, 0,UDPTCPNotifyProc, this);
#else
#ifdef _CARBON
	HX_RESULT theErr = ::OTAsyncOpenInternetServicesInContext(kDefaultInternetServicesPath,
			0,::NewOTNotifyUPP(TCPNotifyProc), this, NULL);
#else
	HX_RESULT theErr = ::OTAsyncOpenInternetServices(kDefaultInternetServicesPath,
			0,TCPNotifyProc, this);
#endif
#endif
	if (theErr != HXR_OK) 
		theErr = HXR_BIND;
	
	return theErr;
}

void
OT_TCP::close_resolver(void)
{
	//dprintf("this:%X OT_TCP::close_resolver\n", this);
	if(mDNSRef != 0)
	{
		::OTRemoveNotifier(mDNSRef);	// remove the DNR notification function
		::OTCloseProvider(mDNSRef);		// close the DNR endpoint
		mDNSRef = 0;
	}
}


HX_RESULT
OT_TCP::open_socket(void)
{
	//dprintf("this:%X OT_TCP::open_socket\n", this);
	HX_RESULT theErr = HXR_OK;
		
	// Open TCP endpoint
	mState = TCP_STATE_OPEN_SOCKET;
#ifdef USE_DEFERRED_IMPL	
	theErr = ::OTAsyncOpenEndpoint(OTCreateConfiguration(kTCPName), 0,NULL, UDPTCPNotifyProc, this);
#else
#ifdef _CARBON
	theErr = ::OTAsyncOpenEndpointInContext(OTCreateConfiguration(kTCPName), 0,NULL,
			::NewOTNotifyUPP(TCPNotifyProc), this, NULL);
#else
	theErr = ::OTAsyncOpenEndpoint(OTCreateConfiguration(kTCPName), 0,NULL,
			TCPNotifyProc, this);
#endif
#endif
								 
	return theErr;
}

 HX_RESULT
 OT_TCP::do_bind (void)
{
	//dprintf("this:%X OT_TCP::do_bind\n", this);
	HX_RESULT	theErr = HXR_OK;
			
	mState = TCP_STATE_BIND_SOCKET;

	mBindRet.addr.maxlen = sizeof(struct InetAddress);
	mBindRet.addr.len = sizeof(struct InetAddress);
	mBindRet.addr.buf = (unsigned char *) &mAddr;

	// bind provider to return structure
	theErr = ::OTBind(mRef, nil, &mBindRet);
	
	if(theErr == kOTNoDataErr) 
		theErr = HXR_OK;
	
	if(theErr)
		theErr = HXR_BIND;

	return theErr;	
}

 HX_RESULT
 OT_TCP::do_connect(void)
{
	//dprintf("this:%X OT_TCP::do_connect\n", this);
	mState = TCP_STATE_CONNECT_SOCKET;

	::memset(&mCall,0,sizeof(TCall));
	mCall.addr.buf = (UInt8 *)&mAddr;
	mCall.addr.maxlen = sizeof(InetAddress);
	mCall.addr.len = sizeof(InetAddress);
	
	// set up address of remote host
	::OTInitInetAddress((InetAddress*)mCall.addr.buf, mRemotePort, mRemoteHost);
	
	// connect client to remote endpoint
	HX_RESULT theErr = ::OTConnect(mRef, &mCall, nil);
	if(theErr == kOTNoDataErr) 
		theErr = HXR_OK;

	if(theErr)
		theErr = HXR_SERVER_DISCONNECTED;
		
	return theErr;
}

/*----------------------------------------------------------------------------
	TCPNotifyProc 
	
	Open Transport notifier proc for TCP streams.
	
	Entry:	s = pointer to stream.
			code = OT event code.
			result = OT result.
			cookie = OT cookie.
----------------------------------------------------------------------------*/

pascal void OT_TCP::TCPNotifyProc (

	 void 			*stream, 
	 OTEventCode 	code,
	 OTResult 		result, 
	 void 			*cookie)
	 
{
	//dprintf("this:%X OT_TCP::TCPNotifyProc cookie:%X code:%X result:%X\n", stream, cookie, code, result);
	HX_RESULT theErr = HXR_OK;
	
	OT_TCP* s =  (OT_TCP*) stream;
	
	HXMM_INTERRUPTON();
	s->ProcessCmd(code, result, cookie);
	HXMM_INTERRUPTOFF();
	return;	
}


void OT_TCP::ProcessCmd(OTEventCode code, OTResult result, void* cookie)	 
{
	HX_RESULT theErr = HXR_OK;
	switch (code) 
	{
		case T_OPENCOMPLETE:
		{
			//dprintf("this:%X T_OPENCOMPLETE ", this);
			// check if T_OPENCOMPLETE was for DNS or socket
			if(mState == TCP_STATE_OPEN_RESOLVER)
			{
				//dprintf(" TCP_STATE_OPEN_RESOLVER\n");
				theErr = result;
				if(!theErr)
				{
					// save DNS provider endpoint ref
					mDNSRef = (InetSvcRef) cookie;
					mDNSOpen = TRUE;
					theErr = resolve_address();
				}
				else
					theErr = HXR_BIND;
			}
			else if(mState == TCP_STATE_OPEN_SOCKET)
			{	
//				DebugWrite("T_OPENCOMPLETE  TCP_STATE_OPEN_SOCKET");
				//dprintf(" TCP_STATE_OPEN_SOCKET\n");
			
				theErr = result;
				if(!theErr)
				{
					// save TCP provider endpoint ref
					mRef = (EndpointRef) cookie;
					mSocketOpen = TRUE;
					theErr = do_bind();
				}
				else
					theErr = HXR_SOCKET_CREATE;
			}
			else if (mState == TCP_STATE_OPEN_LISTEN)
			{
				//dprintf(" TCP_STATE_OPEN_LISTEN\n");
				theErr = result;
				if ( !theErr )
				{
				    mRef = (EndpointRef) cookie;
				    mSocketOpen = TRUE;
				    mCode = code;
				    mComplete = true;
				}
				else
					theErr = HXR_SOCKET_CREATE;
			}
			else if (mState == TCP_STATE_OPEN_ACCEPT)
			{
				//dprintf(" TCP_STATE_OPEN_ACCEPT\n");
				theErr = result;
				if ( !theErr )
				{
				    mRef = (EndpointRef) cookie;
				    mSocketOpen = TRUE;
				    mCode = code;
				    mState = TCP_STATE_READY;
				    mComplete = true;
				}
				else
					theErr = HXR_SOCKET_CREATE;
			}
			break;
		}
			
		case T_DNRSTRINGTOADDRCOMPLETE:
		case T_DNRADDRTONAMECOMPLETE:
		{

//			DebugWrite("T_DNRSTRINGTOADDRCOMPLETE");
			//dprintf("this:%X T_DNRSTRINGTOADDRCOMPLETE\n", this);

			theErr = result;
			if(!theErr)
			{
				// extract DNS result from InetHostInfo struct
				mRemoteHost = mHInfoOT.addrs[0];
				
				// close the DNS endpoint
//				close_resolver();
				
				// open a TCP socket
				// next message should be T_OPEN_COMPLETE
				theErr = open_socket();
 			}
			else
				theErr = HXR_BIND;
				
			break;
		}
		
		case T_BINDCOMPLETE:
		{
//			DebugWrite("T_BINDCOMPLETE");
			//dprintf("this:%X T_BINDCOMPLETE ", this); 
			if ( mState == TCP_STATE_BIND_LISTEN )
			{
				//dprintf(" - Listen\n");
				theErr = result;
				if ( !theErr )
				{
					mCode = code;
					mComplete = true;
					
				}
				else
				{
					theErr = HXR_BIND;
				}
				
			}
			else
			{
				//dprintf(" - Regular\n");
				theErr = result;
				if(!theErr)
				{
					mComplete = true;
					
					// get inet address
					InetAddress *inetAddr = (InetAddress*)mBindRet.addr.buf;
		
					// save local port we are bound to
					mLocalPort = inetAddr->fPort;

					// now connect to the remote host
					theErr = do_connect();
				}
				else
					theErr = HXR_BIND;
			}
					
			break;
		}

		case T_CONNECT:
//			DebugWrite("T_CONNECT");
			//dprintf("this:%X T_CONNECT\n", this);

			theErr = result;
			if(!theErr)
				theErr = ::OTRcvConnect(mRef, nil);
				
			if(!theErr)
			{
				mConnectionOpen = TRUE;
				mWriteReady = TRUE;
				mCallBack->Func(CONNECT_NOTIFICATION); //z
			}
			else
				theErr = HXR_SERVER_DISCONNECTED;
				
			break;

		case T_DISCONNECT:
//			DebugWrite("T_DISCONNECT");
			//dprintf("this:%X T_DISCONNECT\n", this);

			/* Other side has aborted. */
			mCode = code;
			mOtherSideHasClosed = true;
			mComplete = true;
			mConnectionOpen = FALSE;
			mWeHaveClosed = TRUE; //cz
			mStartedReceivingData = FALSE;
			mWriteReady = FALSE;
			::OTRcvDisconnect(mRef, NULL);
			if (mCallBack) mCallBack->Func(CONNECT_NOTIFICATION, FALSE); // rml cr zach, rahul
			break;
			
		case T_ORDREL:
//			DebugWrite("T_ORDREL");
			//dprintf("this:%X T_ORDREL\n", this);
			
			/* Other side has closed. Close our side if necessary. */
			mCode = code;
			mOtherSideHasClosed = true;
			mComplete = true;
			mConnectionOpen = FALSE;
			mStartedReceivingData = FALSE;
			mWriteReady = FALSE;
			::OTRcvOrderlyDisconnect(mRef);
			if (!mWeHaveClosed) 
			{
				::OTSndOrderlyDisconnect(mRef);
				mWeHaveClosed = true;
			}
			if (mClosing) mRelease = true;
			break;
			
		case T_DATA:
//			DebugWrite("T_DATA");		
			//dprintf("this:%X T_DATA\n", this);
			mStartedReceivingData = TRUE;
			if (mClosing) 
			{
				/* Consume and discard response to "QUIT" comand. */
				do 
				{
					result = ::OTRcv(mRef, mBuf, kBufSize, nil);
				} while (result >= 0);
			}
			else if ( mState == TCP_STATE_ACCEPT_PENDING )
			{
				//dprintf("TCP_STAT_ACCEPT_PENDING - hold data.\n");
				mDataReady = TRUE;
			}
			else
			{
				mDataArrived = TRUE;
				if(mCallBack) 
//					mCallBack->callback_task(HX_TCP_CALLBACK);
					mCallBack->Func(READ_NOTIFICATION);
			}
			break;

		case T_DISCONNECTCOMPLETE:
		case T_PASSCON:
			//dprintf("this:%X T_PASSCON ", this);
//			DebugWrite("T_DISCONNECTCOMPLETE");
			if ( mState == TCP_STATE_ACCEPT_PENDING )
			{
				//dprintf("TCP_STATE_ACCEPT_PENDING\n");
				mConnectionOpen = TRUE;
				mWriteReady = TRUE;
				mOtherSideHasClosed = false;
				mState = TCP_STATE_READY;
				if ( mDataReady )
				{
					//dprintf("Data is waiting so give notification\n");
					mDataArrived = TRUE;
					if ( mCallBack )
						mCallBack->Func(READ_NOTIFICATION);
				}
			}
			else
			{
				//dprintf("Regular\n");
				mComplete = true;
				mCode = code;
				mResult = result;
				mCookie = cookie;
			}
			break;

			
		case T_GODATA:
//			DebugWrite("T_GODATA");		
			//dprintf("this:%X T_GODATA\n", this);		
			mDataFlowOn = FALSE;
			mWriteReady = TRUE;
			break;
		case T_LISTEN:
			//dprintf("this:%X T_LISTEN\n", this);
			::memset(&mNewAddr,0,sizeof(TCall));
			mCall.addr.buf = (UInt8 *)&mNewAddr;
			mCall.addr.maxlen = sizeof(InetAddress);
			mCall.addr.len = sizeof(InetAddress);
			//HX_ASSERT(mState == TCP_SOCKET_LISTEN);
			HX_ASSERT(mNewConnection);
			EndpointRef pRef;
#ifdef _CARBON
			mNewConnection->GetEndpoint((void*&)pRef);
#else
			mNewConnection->GetEndpoint((void*)pRef);
#endif
			if ( pRef )
			{
			    theErr = OTListen(mRef,&mCall);
			    if ( !theErr )
			    {
				if ( !theErr )
				{
				    theErr = OTAccept(mRef, pRef, &mCall);
				}
			    }
			}
			else
			{
				// some how we got messed up
				// not much we can do... we will
				// just ignor the listen request...
			}
			break;
		case T_ACCEPTCOMPLETE:
			{
				//dprintf("this:%X T_ACCEPTCOMPLETE\n", this);
				mCallBack->Func(ACCEPT_NOTIFICATION, 
						TRUE, mNewConnection);
				// it is the callback's job to call done...
				HX_RELEASE(mNewConnection);
				mNewConnection = conn::actual_new_socket(HX_TCP_SOCKET);
				mNewConnection->AddRef();
				// we are in deffered task time so this will not finish
				// until wait for event is called again...  therefore
				// we pass in false for wait.  We will wait at the
				// beginning to GetEndpoint() if it hasn't finished yet.
				HX_RESULT theErr = mNewConnection->SetupEndpoint(FALSE);
			}
			break;
		case kOTProviderIsClosed:
			{
				// This event is triggered by OpenTransport when the network is gone
				// i.e. network cable has been unpluged
				mCode = code;
				mResult = result;
				mOtherSideHasClosed = true;
				mComplete = true;
				mConnectionOpen = FALSE;
				mWeHaveClosed = TRUE;
				mStartedReceivingData = FALSE;
				mWriteReady = FALSE;
				::OTRcvDisconnect(mRef, NULL);
				if (mCallBack) mCallBack->Func(CONNECT_NOTIFICATION, FALSE);
			}
			break;
		default:
			//dprintf("this:%X Uncaugt message - %X\n", this, code);
			break;
	}
	
	mAsyncError = theErr;
}

HX_RESULT OT_TCP::SetupEndpoint(HXBOOL bWait)
{
	//dprintf("this:%X OT_TCP::SetupEndpoint\n", this);
	// Open TCP endpoint
	mState = TCP_STATE_OPEN_ACCEPT;
	mComplete = false;
	mConnectionOpen = FALSE;
	mDataFlowOn = FALSE;
	mDataArrived = FALSE;
	mDataReady = FALSE;
	mAsyncError = HXR_OK;

#ifdef USE_DEFERRED_IMPL	
	HX_RESULT theErr = ::OTAsyncOpenEndpoint(OTCreateConfiguration(kTCPName), 0,NULL, UDPTCPNotifyProc, this);
#else
#ifdef _CARBON
	HX_RESULT theErr = ::OTAsyncOpenEndpointInContext(OTCreateConfiguration(kTCPName),
			0,NULL, (OTNotifyUPP)TCPNotifyProc, this, NULL);
#else
	HX_RESULT theErr = ::OTAsyncOpenEndpoint(OTCreateConfiguration(kTCPName),
			0,NULL, TCPNotifyProc, this);
#endif
#endif
	if ( theErr )
	{
		mLastError = theErr;
		theErr = HXR_SOCKET_CREATE;
	}
	// wait for open completion.. if we were called from deffered task time
	// or Interupt time we don't want to wait though...  Because the creation
	// of the end point will not finish until control is passed back to the
	// application.
	if( bWait )
	{
		if ( !theErr)
		{
			theErr = OTWait();
			if(theErr == kOTNoDataErr) theErr = HXR_OK;
			
			if(theErr || mCode != T_OPENCOMPLETE) 
			{
				theErr = theErr ? theErr : HXR_SOCKET_CREATE;
				mLastError = theErr;
				theErr = HXR_SOCKET_CREATE;
			}
		}
	}
	return theErr;
}

HX_RESULT OT_TCP::GetEndpoint(REF(void*) pRef)
{
	////dprintf("this:%X OT_TCP::GetEndpoint\n", this);
	HX_RESULT theErr = HXR_OK;
	if ( mState == TCP_STATE_OPEN_ACCEPT )
	{
		theErr = OTWait();
		if(theErr == kOTNoDataErr) theErr = HXR_OK;
		if ( theErr || mCode != T_OPENCOMPLETE )
		{
			theErr = theErr ? theErr : HXR_SOCKET_CREATE;
			mLastError = theErr;
			return HXR_SOCKET_CREATE;
		}
	}
	if ( mRef )
	{
		mState = TCP_STATE_ACCEPT_PENDING;
		pRef = (void*)mRef;
		return HXR_OK;
	}
	else
	{
		return HXR_NOT_INITIALIZED;
	}
}
