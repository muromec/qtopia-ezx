/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ot_udp.cp,v 1.5 2007/07/06 21:57:58 jfinnecy Exp $
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
#include "hxbuffer.h"
#include "timebuff.h"
#include "hxtick.h"
#include "netbyte.h"
#include "OT_UDP.h"
#include "hxerrors.h"
#include "MWDebug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "hxmm.h"

//#define USE_DEFERRED_IMPL 1

OT_UDP::OT_UDP (void)
	
	{ /* begin OT_UDP */
		
		m_pInBuffer = new char[TCP_BUF_SIZE];
		m_bIsBroadcastEnabled = FALSE;
	} /* end OT_UDP */
	
OT_UDP::~OT_UDP (void)
	
	{ /* begin ~OT_UDP */
	
		Cleanup();
		
	} /* end ~OT_UDP */
	
HX_RESULT
OT_UDP::init(UINT32 local_addr, UINT16 port, UINT16 blocking)
{
	HX_RESULT theErr = Create();
	if(!theErr) theErr = ActiveOpen(port);
	if(theErr) Cleanup();
	
	return theErr;
}

void
OT_UDP::done(void)
{
	Cleanup();
}
	
/*----------------------------------------------------------------------------
	Create
	
	Create a UDP endpoint.
	
	Exit:	function result = error code.
----------------------------------------------------------------------------*/

OSErr OT_UDP::Create(void)
{
	OSErr theErr = HXR_OK;
	
	// clear completion flag
	mComplete = false;
	
	// open UDP endpoint
#ifdef USE_DEFERRED_IMPL	
	theErr = OTAsyncOpenEndpoint(OTCreateConfiguration(kUDPName), 0, nil, UDPTCPNotifyProc, this);
#else
#ifdef _CARBON
	theErr = OTAsyncOpenEndpointInContext(OTCreateConfiguration(kUDPName), 0, nil,
			::NewOTNotifyUPP(UDPNotifyProc), this, NULL);
#else
	theErr = OTAsyncOpenEndpoint(OTCreateConfiguration(kUDPName), 0, nil,
			UDPNotifyProc, this);
#endif
#endif
	
	// wait for completion
	if(!theErr) theErr = OTWait();

	// check for errors
	if (theErr || mCode != T_OPENCOMPLETE)
		theErr = HXR_SOCKET_CREATE;
	
	if(!theErr)
	{
		mRef = (EndpointRef) mCookie;	// save UDP provider reference
		mDataArrived = FALSE;
		mDataFlowOn = FALSE;
	}
	
#if 0
	if (!theErr)
	{
	 UINT32 uBufDepth = 0;
	 // XTI_RCVBUF : size of internal buffer
	 // XTI_RCVLOWAT : minimum accumulated size for notification
	OT_net::GetFourByteOption(mRef, XTI_GENERIC, XTI_RCVBUF, &uBufDepth);
	OT_net::GetFourByteOption(mRef, XTI_GENERIC, XTI_RCVLOWAT, &uBufDepth);
	
	OT_net::SetFourByteOption(mRef, XTI_GENERIC, XTI_RCVBUF, 64000);
	OT_net::SetFourByteOption(mRef, XTI_GENERIC, XTI_RCVLOWAT, 32000);
	OT_net::GetFourByteOption(mRef, XTI_GENERIC, XTI_RCVBUF, &uBufDepth);
	OT_net::GetFourByteOption(mRef, XTI_GENERIC, XTI_RCVLOWAT, &uBufDepth);
	OT_net::GetFourByteOption(mRef, XTI_GENERIC, XTI_RCVLOWAT, &uBufDepth);
	}
#endif	
		
	if(theErr) 	// error occured close provider
	{
		if(mRef != 0) OTCloseProvider(mRef);
		mRef = 0;
	}

	mLastError = theErr;
	
	return theErr;
}

		
/*----------------------------------------------------------------------------
	Release 
	
	Release a UDP stream.
	
	Exit:	function result = error code.
	
	Any active connection is also aborted, if necessary, before releasing 
	the stream.
	
----------------------------------------------------------------------------*/

OSErr OT_UDP::Cleanup (void)
{
	OSErr theErr = HXR_OK;
	Boolean abort;

	abort = !mOtherSideHasClosed || !mWeHaveClosed;
	
	if(mRef)
	{
		OTRemoveNotifier(mRef);
		theErr = OTCloseProvider(mRef);
		mRef = 0;
		mConnectionOpen = FALSE;
		mOtherSideHasClosed = TRUE;
		mWeHaveClosed = TRUE;
	}
	
	HX_VECTOR_DELETE(m_pInBuffer);
	
	mLastError = theErr;
	return theErr;
}

/*----------------------------------------------------------------------------
	ActiveOpen 
	
	Open an active stream.
	
			addr = IP address of server.
			port = port number of service.
	
	Exit:	function result = error code.
----------------------------------------------------------------------------*/

 OSErr OT_UDP::ActiveOpen (
 
	UINT16 port)
	
{
	OSErr theErr = HXR_OK;
	InetAddress reqAd,retAd;
	TBind req,ret;
	
	// get Inet address of port
	OTInitInetAddress(&reqAd, port, (InetHost) 0);

	// bind udp to current address and requested port
	req.addr.maxlen = sizeof(struct InetAddress);
	req.addr.len = sizeof(struct InetAddress);
	req.addr.buf = (unsigned char *) &reqAd;
	req.qlen = 1;				// don't care for udp
	ret.addr.maxlen = sizeof(struct InetAddress);
	ret.addr.len = sizeof(struct InetAddress);
	ret.addr.buf = (unsigned char *) &retAd;

	// clear completion flag
	mComplete = false;
	
	// bind provider to return structure
	theErr = OTBind(mRef, &req, &ret);
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
		mLocalPort = reqAd.fPort;
		mConnectionOpen = TRUE;
		mOtherSideHasClosed = FALSE;
		mWeHaveClosed = FALSE;
	}
	
	return theErr;
}

/*----------------------------------------------------------------------------
	write 
	
	write data on a UDP stream.
	
	Entry:	data = pointer to data to send.
			len = length of data to send.
	
	Exit:	function result = error code.
----------------------------------------------------------------------------*/

HX_RESULT
OT_UDP::writeto (
	void 	*buf,
	UINT16 	*size, 
	ULONG32 addr,
	UINT16 	port)
{

	OSStatus	result;
	TUnitData	unitdata;
	InetAddress sin;
	TOption		option;
	HX_RESULT	theErr;
	
	::OTInitInetAddress(&sin, port, addr);

	unitdata.addr.len = sizeof(struct InetAddress);
	unitdata.addr.buf = (UInt8*) &sin;
	unitdata.udata.len = *size;
	unitdata.udata.buf = (UInt8*) buf;

 	if(m_bIsBroadcastEnabled)
 	{
 		unitdata.opt.len = sizeof(option);
 		option.len = kOTFourByteOptionSize;
 		option.level = INET_IP;
#ifdef _MAC_MACHO
 		option.name = kIP_BROADCAST;
#else
 		option.name = IP_BROADCAST;
#endif
 		option.status = 0;
 		option.value[0] = 1;
 		unitdata.opt.buf = (UInt8*) &option;
 	}
 	else
 	{
 		unitdata.opt.len = 0;
 		unitdata.opt.maxlen = 0;
 		unitdata.opt.buf = 0;
 	}

	result = ::OTSndUData(mRef,&unitdata);
	
	if(result == 0)
	{
		return HXR_OK;
	}

	switch(result)
	{
		case kOTFlowErr:
			mDataFlowOn = TRUE;
			theErr = HXR_WOULD_BLOCK;
			break;
			
		case kENOMEMErr:
		case kEBUSYErr:
		case kOTOutStateErr:
		case kEWOULDBLOCKErr:
		case kOTStateChangeErr:
			theErr = HXR_WOULD_BLOCK;
			break;

		case kOTLookErr:
			{
                        OSStatus lookErr = ::OTLook(mRef);
			
			TUDErr UDErr;
		
			if(lookErr == T_UDERR)
			{
				UInt8 buf1[256];
				UInt8 buf2[256];
				
				UDErr.addr.maxlen = 256;
				UDErr.addr.buf = buf1;
				UDErr.opt.maxlen = 256;
				UDErr.opt.buf = buf2;
				lookErr = ::OTRcvUDErr(mRef, &UDErr);
				theErr = HXR_NET_WRITE;
			}
			else
			{
				theErr =  HXR_NET_WRITE;
				break;
			}
			}
                        break;
			
		default:
			theErr =  HXR_NET_WRITE;
			break;
	}
	
	*size = 0;
	mLastError = theErr;
	
	return theErr;
}




/*******************************************************************************
** read
********************************************************************************/

HX_RESULT
OT_UDP::read (void *data, UINT16 *len)
{
	return HXR_NOTIMPL;
}

HX_RESULT
OT_UDP::readfrom (REF(IHXBuffer*)   pBuffer,
				  REF(UINT32)	    ulAddress,
				  REF(UINT16)	    ulPort)
{
	UINT16		size = 0;
	OSStatus	theErr = HXR_OK;
	TUnitData	unitdata;
	OTFlags		flags;
	InetAddress	sin;

	pBuffer = NULL;
	ulAddress = 0;
	ulPort = 0;

	mLastError = HXR_OK;
	
	if(!mDataArrived)
	{
		return HXR_WOULD_BLOCK;
	}
	
	unitdata.opt.len = 0;
	unitdata.addr.len = 0;
	unitdata.udata.len = 0;
	unitdata.addr.maxlen = sizeof(struct InetAddress);
	unitdata.opt.maxlen = 0;
	unitdata.opt.buf = 0;
	unitdata.udata.maxlen = TCP_BUF_SIZE;			// used to be 256
	unitdata.udata.buf = (UInt8*)m_pInBuffer;
	unitdata.addr.buf = (UInt8*) &sin;

	theErr = OTRcvUData(mRef,&unitdata, &flags);

	if (theErr == HXR_OK)
	{
		size = unitdata.udata.len;
		
		/* I have seen the size to be a really large value some times.
		 * This was before I added initializaion of
 	     *  unitdata.opt.len = 0;
	     *  unitdata.addr.len = 0;
	     *  unitdata.udata.len = 0;
	     *
	     * It looks like sometimes OTRecvData returns no error and DOES NOT
	     * set the udata.len value. We now make sure that we have indeed
	     * received some data before creating an IHXBuffer
	     *
	     * XXXRA
	     */
		 
		HX_ASSERT(size > 0);		
		if (size > 0)
		{
		    CHXTimeStampedBuffer* pTimeBuffer = new CHXTimeStampedBuffer;
		    pTimeBuffer->AddRef();
		    pTimeBuffer->SetTimeStamp(HX_GET_TICKCOUNT());
		    pTimeBuffer->Set((UCHAR*)m_pInBuffer, size);
		    pBuffer = (IHXBuffer*) pTimeBuffer;

		    ulAddress = DwToHost(sin.fHost);
		    ulPort = WToHost(sin.fPort);
		}
	}	
	else if(theErr == kOTNoDataErr)
	{
		theErr = HXR_NO_DATA;
		// xxxbobclark This used to return HXR_OK if there was no data.
		// But now there's threaded networking code that needs to rely
		// on this returning HXR_NO_DATA if there's really no data.
	}
	else if(theErr == kEWOULDBLOCKErr)
		theErr = HXR_WOULD_BLOCK;
	else
		theErr = HXR_SERVER_DISCONNECTED;

	mLastError = theErr;
	return theErr;
}
/*
HX_RESULT
OT_UDP::listen (UINT16 	backlog)
{
	return(HXR_INVALID_OPERATION);
}
*/	
HX_RESULT
OT_UDP::connect(

	const char	*host, 
	UINT16 		port,
	UINT16 		blocking,
	ULONG32	ulPlatform)

{
	return(HXR_INVALID_OPERATION);
}
	

/* join_multicast_group() has this socket join a multicast group */
HX_RESULT
OT_UDP::join_multicast_group (ULONG32 multicastHost, ULONG32 if_addr)
{
	OSStatus		theErr = kOTNoError;

	// mLocalPort is set by the init() function
	// ::OTInitInetAddress(&mMulticastAddr, mLocalPort, multicastHost);

	// since we are already bound to an IP addr on our system
	// (bind occurred in the init() function) we can now Let 
	// IP know to listen for this multicast IP address on all interfaces.
	
	TOptMgmt 			optReq;
	UInt8	 			optBuffer[ kOTOptionHeaderSize + sizeof(TIPAddMulticast) ];
	ULONG32				optLength = kOTOptionHeaderSize + sizeof(TIPAddMulticast);
	TOption*	 		opt = (TOption*)optBuffer;
	TIPAddMulticast*	addopt = (TIPAddMulticast*)opt->value; 
	
	optReq.flags = T_NEGOTIATE;
	optReq.opt.len = optLength;
	optReq.opt.maxlen = optLength;
	optReq.opt.buf = (UInt8*) optBuffer;
	
	opt->level = INET_IP;
#ifdef _MAC_MACHO
	opt->name = kIP_ADD_MEMBERSHIP;
#else
	opt->name = IP_ADD_MEMBERSHIP;
#endif
	opt->len = optLength;
	opt->status = 0;
	
	addopt->multicastGroupAddress = multicastHost;
	addopt->interfaceAddress = if_addr;
	
	// clear completion flag
	mComplete = false;

	theErr = ::OTOptionManagement(mRef,&optReq, &optReq);

	// wait for completion
	if(!theErr) 
		theErr = OTWait();
	
	if(theErr)
		theErr = HX_MULTICAST_JOIN_ERROR;
		
	return theErr;
	
}

HX_RESULT
OT_UDP::leave_multicast_group(ULONG32 multicastHost, ULONG32 if_addr)
{
	HX_RESULT theErr = HXR_OK;
	
	TOptMgmt 			optReq;
	UInt8	 			optBuffer[ kOTOptionHeaderSize + sizeof(TIPAddMulticast) ];
	ULONG32				optLength = kOTOptionHeaderSize + sizeof(TIPAddMulticast);
	TOption*	 		opt = (TOption*)optBuffer;
	TIPAddMulticast*	addopt = (TIPAddMulticast*)opt->value; 
	
	optReq.flags = T_NEGOTIATE;
	optReq.opt.len = optLength;
	optReq.opt.maxlen = optLength;
	optReq.opt.buf = (UInt8*) optBuffer;
	
	opt->level = INET_IP;
#ifdef _MAC_MACHO
	opt->name = kIP_DROP_MEMBERSHIP;
#else
	opt->name = IP_DROP_MEMBERSHIP;
#endif
	opt->len = optLength;
	opt->status = 0;
	
	addopt->multicastGroupAddress = multicastHost;
	addopt->interfaceAddress = if_addr;
	
	// clear completion flag
	mComplete = false;

	theErr = ::OTOptionManagement(mRef,&optReq, &optReq);

	// wait for completion
	if(!theErr) 
		theErr = OTWait();

	if(theErr)
		return HX_GENERAL_MULTICAST_ERROR;
	
	return HXR_OK;
}


HX_RESULT
OT_UDP::set_broadcast(HXBOOL enable)
{
	OSStatus		theErr = kOTNoError;
 	mComplete = false;
   	m_bIsBroadcastEnabled = enable;		
 	return HXR_OK;
}
 	

/*----------------------------------------------------------------------------
	UDPNotifyProc 
	
	Open Transport notifier proc for UDP streams.
	
	Entry:	s = pointer to UDP stream.
			code = OT event code.
			result = OT result.
			cookie = OT cookie.
----------------------------------------------------------------------------*/

pascal void OT_UDP::UDPNotifyProc (

	 void *stream, 
	 OTEventCode code,
	 OTResult result, 
	 void *cookie )
	 
{
	OT_UDP* s =  (OT_UDP*) stream;
	
	HXMM_INTERRUPTON();
	s->ProcessCmd(code, result, cookie);
	HXMM_INTERRUPTOFF();
	return;		
}

void
OT_UDP::ProcessCmd(OTEventCode code, OTResult result, void* cookie)
{
	switch (code) 
	{
		case T_DISCONNECT:
			/* Other side has aborted. */
			mOtherSideHasClosed = true;
			mComplete = true;
			mConnectionOpen = FALSE;
			MWDebugPStr("\pT_DISCONNECT");
			
	        if(mCallBack) mCallBack->Func(CONNECT_NOTIFICATION, FALSE);
			break;
			
		case T_ORDREL:
			/* Other side has closed. Close our side if necessary. */
			mOtherSideHasClosed = true;
			mComplete = true;
			mConnectionOpen = FALSE;
			if (mClosing) mRelease = true;
			MWDebugPStr("\pT_ORDREL");
			break;
			
		case T_DATA:
			mDataArrived = TRUE;
			//if(mCallBack) mCallBack->callback_task(HX_UDP_CALLBACK);
			if(mCallBack) mCallBack->Func(READ_NOTIFICATION);
			break;

		case T_BINDCOMPLETE:
		case T_CONNECT:
		case T_PASSCON:
			mComplete = true;
			mCode = code;
			mResult = result;
			mCookie = cookie;
			break;

		case T_OPENCOMPLETE:
			mConnectionOpen = TRUE;
			mComplete = true;
			mCode = code;
			mResult = result;
			mCookie = cookie;
			if(mCallBack) mCallBack->Func(CONNECT_NOTIFICATION);
			break;
		
		case T_GODATA:
			mDataFlowOn = FALSE;
			MWDebugPStr("\pT_GODATA");
			break;

		case T_OPTMGMTCOMPLETE:
			mComplete = true;
			mCode = code;
			mResult = result;
			mCookie = cookie;
			break;
	}

	return;
}
