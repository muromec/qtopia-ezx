/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: oterrs.h,v 1.4 2007/07/06 20:43:44 jfinnecy Exp $
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

#ifndef _OTERRS_H_
#define _OTERRS_H_

	#undef EPERM
	#undef ENOENT
	#undef ENORSRC
	#undef EINTR
	#undef EIO
	#undef ENXIO
	#undef E2BIG
	#undef EBADF
	#undef EAGAIN
	#undef ENOMEM
	#undef EACCES
	#undef EFAULT
	#undef EBUSY
	#undef EEXIST
	#undef ENODEV
	#undef EINVAL
	#undef ENOTTY
	#undef ERANGE
	#undef ESRCH
	#undef EPIPE

	enum
	{
		EPERM			= 1,		/* Permission denied					*/
		ENOENT			= 2,		/* No such file or directory			*/
		ENORSRC 	 	= 3,		/* No such resource						*/
		EINTR			= 4,		/* Interrupted system service			*/
		EIO 			= 5,		/* I/O error							*/
		ENXIO			= 6,		/* No such device or address			*/
		EBADF		 	= 9,		/* Bad file number						*/
		EAGAIN			= 11,		/* Try operation again later			*/
		ENOMEM			= 12,		/* Not enough space						*/
		EACCES			= 13,		/* Permission denied					*/
		EFAULT			= 14,		/* Bad address							*/
		EBUSY			= 16,		/* Device or resource busy				*/
		EEXIST			= 17,		/* File exists							*/
		ENODEV			= 19,		/* No such device						*/
		EINVAL			= 22,		/* Invalid argument						*/
		ENOTTY			= 25,		/* Not a character device				*/
		EPIPE			= 32,		/* Broken pipe							*/
		ERANGE			= 34,		/* Math result not representable		*/
#ifndef EDEADLK
		EDEADLK			= 35,		/* Call would block so was aborted		*/ 
#endif
		EWOULDBLOCK		= EDEADLK,	/* Or a deadlock would occur			*/
		EALREADY		= 37,
		ENOTSOCK		= 38,		/* Socket operation on non-socket		*/
		EDESTADDRREQ	= 39,		/* Destination address required			*/
		EMSGSIZE		= 40,		/* Message too long						*/
		EPROTOTYPE		= 41,		/* Protocol wrong type for socket		*/
		ENOPROTOOPT		= 42,		/* Protocol not available				*/
		EPROTONOSUPPORT	= 43,		/* Protocol not supported				*/
		ESOCKTNOSUPPORT	= 44,		/* Socket type not supported			*/
		EOPNOTSUPP		= 45,		/* Operation not supported on socket	*/
		EADDRINUSE		= 48,		/* Address already in use				*/
		EADDRNOTAVAIL	= 49,		/* Can't assign requested address		*/
	
		ENETDOWN		= 50,		/* Network is down						*/
		ENETUNREACH		= 51,		/* Network is unreachable				*/
		ENETRESET		= 52,		/* Network dropped connection on reset	*/
		ECONNABORTED	= 53,		/* Software caused connection abort		*/
		ECONNRESET		= 54,		/* Connection reset by peer				*/
		ENOBUFS			= 55,		/* No buffer space available			*/
		EISCONN			= 56,		/* Socket is already connected			*/
		ENOTCONN		= 57,		/* Socket is not connected				*/
		ESHUTDOWN		= 58,		/* Can't send after socket shutdown		*/
		ETOOMANYREFS	= 59,		/* Too many references: can't splice	*/
		ETIMEDOUT		= 60,		/* Connection timed out					*/
		ECONNREFUSED	= 61,		/* Connection refused					*/
	
		EHOSTDOWN		= 64,		/* Host is down							*/
		EHOSTUNREACH	= 65,		/* No route to host						*/
	
		EPROTO			= 70,
		ETIME			= 71,
		ENOSR			= 72,
		EBADMSG			= 73,
		ECANCEL			= 74,
		ENOSTR			= 75,
		ENODATA			= 76,
		EINPROGRESS		= 77,
	
		ESRCH		 	= 78,
		ENOMSG			= 79,
	
		ELASTERRNO		= 79
	};
#endif/*_OTERRS_H_*/
