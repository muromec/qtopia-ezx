/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ot_tcp.h,v 1.4 2007/07/06 21:57:58 jfinnecy Exp $
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

#include "OT_net.h"

// TCP async socket states
enum
{
	TCP_STATE_CLOSED = 0,
	TCP_STATE_OPEN_RESOLVER,
	TCP_STATE_RESOLVE_DNS,
	TCP_STATE_OPEN_SOCKET,
	TCP_STATE_BIND_SOCKET,
	TCP_STATE_CONNECT_SOCKET,
	TCP_STATE_OPEN_LISTEN,
	TCP_STATE_BIND_LISTEN,
	TCP_STATE_LISTEN,
	TCP_STATE_OPEN_ACCEPT,
	TCP_STATE_READY,
	TCP_STATE_ACCEPT_PENDING,
	TCP_STATE_SOCKET_ERROR
};


class OT_TCP : public OT_net {

	public:

					OT_TCP			(void);
	virtual				~OT_TCP			(void);

		
	virtual	void		done			(void);
	
	virtual HX_RESULT	init			(UINT32		local_addr,
										 UINT16 	port, 
										 UINT16 	blocking=0);
										 
	virtual HX_RESULT	read			(void 		*buf, 
										 UINT16 	*size);

	virtual HX_RESULT	readfrom		(REF(IHXBuffer*)   pBuffer,
							 REF(UINT32)	    ulAddress,
							 REF(UINT16)	    ulPort);
	
	virtual HX_RESULT	write			(void 		*buf,
										 UINT16 	*size);
										 
	virtual HX_RESULT	listen			(ULONG32	ulLocalAddr,
							 UINT16		port,
							 UINT16 	backlog,
							 UINT16		blocking,
							 ULONG32	ulPlatform);

	virtual HX_RESULT	connect			(const char	*host, 
										 UINT16 	port,
										 UINT16 	blocking=0,
										 ULONG32	ulPlatform=0 );
										 
			OSErr		Cleanup			(Boolean orderly = FALSE);

	// Creates a new EndPoint bound to this object.
	virtual HX_RESULT	GetEndpoint(REF(void*) pRef);
	virtual HX_RESULT	SetupEndpoint(HXBOOL bWait);
	static pascal 	void 	TCPNotifyProc  	(void *stream, 
						 OTEventCode code,
	 					 OTResult result, 
	 					 void *cookie);
	protected:
			
			HX_RESULT		open_resolver	(void);
			HX_RESULT		resolve_address	(void);
			void			close_resolver	(void);
			HX_RESULT		do_bind			(void);
			HX_RESULT		do_connect		(void);
			HX_RESULT		open_socket		(void);
			
			void ProcessCmd(OTEventCode code, OTResult result, void* cookie);	 

			
			Boolean			mWriteReady;			//
			InetSvcRef		mDNSRef;
			InetHostInfo 	mHInfoOT;
			Boolean			mDNSOpen;
			Boolean			mSocketOpen;
			UINT16			mState;
			char*			mRemoteHostName;
			InetAddress 	mAddr;
			TCall 			mCall;
			TBind 			mBindRet;			// bind return structure 
			conn*			mNewConnection;
			InetAddress	mNewAddr;
			HXBOOL 		mDataReady;
			
	};

