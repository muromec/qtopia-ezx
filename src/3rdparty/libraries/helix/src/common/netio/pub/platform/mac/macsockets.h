/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: macsockets.h,v 1.5 2004/07/09 18:22:23 hubbe Exp $
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

#if __MWERKS__
#pragma once
#endif

#ifndef __MACSOCKETS__
#define __MACSOCKETS__


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef unsigned char u_char;
typedef unsigned short u_short;

#ifndef _MAC_MACHO
#ifndef mode_t
typedef unsigned long	mode_t;
#endif
#endif

#ifdef OTUNIXERRORS
#undef OTUNIXERRORS
#endif

#define OTUNIXERRORS 1


#ifndef _STDIO
#include <stdio.h>
#endif

#ifndef _MAC_MACHO
#ifndef _STAT
#include <stat.h>
#endif
#endif

#ifndef _UTIME
#include <utime.h>
#endif

#ifndef _MAC_MACHO
#ifndef _UTSNAME
#include <utsname.h>
#endif
#endif

#ifndef _HXTYPES_H_
#include "hxtypes.h"
#endif
/*
// Not defined in OpenTransport.h
#define EMFILE ELASTERRNO + 1
#define ENFILE ELASTERRNO + 2
#define ENOENT ELASTERRNO + 3
#define ENOSPC ELASTERRNO + 4
#define EROFS  ELASTERRNO + 5
#define ESPIPE  ELASTERRNO + 6
*/
/* Definitions related to sockets: types, address families, options */

#define ntohl(x)	(x)
#define ntohs(x)	(x)
#define htonl(x)	(x)
#define htons(x)	(x)

/*
 * Types
 */
#define	SOCK_STREAM		1		/* stream socket */
#define	SOCK_DGRAM		2		/* datagram socket */
#define	SOCK_RAW			3		/* raw-protocol interface */
#define	SOCK_RDM			4		/* reliably-delivered message */
#define	SOCK_SEQPACKET	5		/* sequenced packet stream */

/*
 * Option flags per-socket.
 */
#define	SO_DEBUG			0x0001		/* turn on debugging info recording */
#define	SO_ACCEPTCONN	0x0002		/* socket has had listen() */
#define	SO_REUSEADDR	0x0004		/* allow local address reuse */
#define	SO_KEEPALIVE	0x0008		/* keep connections alive */
#define	SO_DONTROUTE	0x0010		/* just use interface addresses */
#define	SO_BROADCAST	0x0020		/* permit sending of broadcast msgs */
#define	SO_USELOOPBACK	0x0040		/* bypass hardware when possible */
#define	SO_LINGER		0x0080		/* linger on close if data present */
#define	SO_OOBINLINE	0x0100		/* leave received OOB data in line */

/*
 * Additional options, not kept in so_options.
 */
#define SO_SNDBUF		0x1001		/* send buffer size */
#define SO_RCVBUF		0x1002		/* receive buffer size */
#define SO_SNDLOWAT	0x1003		/* send low-water mark */
#define SO_RCVLOWAT	0x1004		/* receive low-water mark */
#define SO_SNDTIMEO	0x1005		/* send timeout */
#define SO_RCVTIMEO	0x1006		/* receive timeout */
#define	SO_ERROR		0x1007		/* get error status and clear */
#define	SO_TYPE		0x1008		/* get socket type */

/*
 * Structure used for manipulating linger option.
 */
struct	linger {
	int	l_onoff;			/* option on/off */
	int	l_linger;		/* linger time */
};

/*
 * Structures returned by network data base library.  All addresses are
 * supplied in host order, and returned in network order (suitable for
 * use in system calls).
 */

#if 0
struct	hostent {
	char	*h_name;	/* official name of host */
	char	**h_aliases;	/* alias list */
	int	h_addrtype;	/* host address type */
	int	h_length;	/* length of address */
	char	**h_addr_list;	/* list of addresses from name server */
#define	h_addr	h_addr_list[0]	/* address, for backward compatiblity */
};

#else

struct	hostent {
	unsigned long *h_addr;
	char	*h_name;				/* official name of host */
};
#endif

#define	PF_UNSPEC		AF_UNSPEC		/* unspecified 									*/
#define	PF_UNIX			AF_UNIX			/* local to host (pipes, portals) 			*/
#define	PF_INET			AF_INET			/* internetwork: UDP, TCP, etc. 				*/
#define	PF_CTB			AF_CTB			/* Apple Comm Toolbox (not yet supported) */
#define	PF_FILE			AF_FILE			/* Normal File I/O (used internally)		*/
#define	PF_PPC			AF_PPC			/* PPC Toolbox										*/
#define	PF_PAP			AF_PAP			/* Printer Access Protocol (client only)	*/
#define	PF_APPLETALK	AF_APPLETALK	/* Apple Talk 										*/

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define	SOL_SOCKET	0xffff		/* options for socket level */

#define	INADDR_ANY  (unsigned long)0x00000000

struct sockaddr 
{
	u_char	sa_len;			/* total length */
	u_char	sa_family;		/* address family */
	char		sa_data[14];	/* actually longer; address value */ /* Flawfinder: ignore */
};


/*
 * Internet address (a structure for historical reasons)
 */
struct in_addr {
	unsigned long s_addr;
};

/*
 * Socket address, internet style.
 */
struct sockaddr_in {
	unsigned char	sin_len;
	unsigned	char	sin_family;
	unsigned short	sin_port;
	struct			in_addr sin_addr;
	char				sin_zero[8]; /* Flawfinder: ignore */
};

extern char 			*inet_ntoa(struct in_addr in);
extern unsigned long inet_addr(const char* cp);
extern struct 			hostent *gethostbyname(char *name);
extern INT16 			gethostname(char *name, INT16 namelen);
struct hostent 		*gethostbyaddr(char *addr, int len, int type);
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __MACSOCKETS__ */
