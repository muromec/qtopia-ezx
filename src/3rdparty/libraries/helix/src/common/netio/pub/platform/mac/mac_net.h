/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mac_net.h,v 1.8 2007/07/06 20:43:58 jfinnecy Exp $
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


//
//	Mac_net.h
//
//	A class to handle basic net functions on the Macintosh computer
//

#ifndef _MAC_NET
#define	_MAC_NET

#include "conn.h"
#include "platform/mac/macsockets.h"

class Mac_net : public conn {

public:

	static	Boolean sHasOT;
	static	Boolean	sHasMacTCP;
	static	Boolean	sNetInitialized;
	
/* 	call new_socket() to automatically create the correct platform specific network object.
	The type parameter may be either HX_TCP_SOCKET or HX_UDP_SOCKET. If new_socket() returns 
	NULL, an error occurred and the conn object was not created. Call last_error() to get the error */
	
	static	Mac_net		*new_socket 	(IUnknown* pContext, UINT16 type);

				   		~Mac_net		(void);

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


	static HX_RESULT get_host_name(char *name, int namelen)
	{
	    return HXR_NOTIMPL;
	}
	static HX_RESULT get_host_by_name(char *name, REF(struct hostent*) pHostent)
	{
	    return HXR_NOTIMPL;
	}

/* 	call done() when you are finished with the socket. Done() will close the socket.
	You may reuse the socket by calling init() or connect() */
	
	virtual void		done			(void) = 0;
	
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

	virtual HX_RESULT	connect			(const 		char *host, 
							 UINT16 	port,
							 UINT16 	blocking=0,
							 ULONG32	ulPlatform=0 ) = 0;
	
	virtual HX_RESULT	blocking		(void)      {return HXR_OK;};
	
	virtual HX_RESULT	nonblocking		(void)	    {return HXR_OK;};
	
	virtual HX_RESULT	read			(void 		*buf, 
							 UINT16 	*size) = 0;
	
	virtual HX_RESULT	readfrom		(REF(IHXBuffer*)   pBuffer,
							 REF(UINT32)	    ulAddress,
							 REF(UINT16)	    ulPort) = 0;

	virtual HX_RESULT	write			(void 		*buf,
							 UINT16 	*size) = 0;
										 
	static	Boolean		CheckForCancel	(void);

	virtual UINT16		get_local_port		(void) { return -1; }

protected:


						Mac_net			(void);
						LONG32	    m_lRefCount;
						
};

#endif // _MAC_NET		


#ifdef __cplusplus
extern "C" {
#endif
INT16	getsockname(int sock, sockaddr* addr, INT16* addr_len);

#ifdef __cplusplus
}
#endif
