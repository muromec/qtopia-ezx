/* ***** BEGIN LICENSE BLOCK *****
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

#ifndef _SOERRS_H_
#define _SOERRS_H_

#include "resid.h"

#define IDS_SOCK_INTR			HX_SO_RES_INIT_ID + 0
#define IDS_SOCK_BADF			HX_SO_RES_INIT_ID + 1
#define IDS_SOCK_ACCES			HX_SO_RES_INIT_ID + 2
#define IDS_SOCK_FAULT			HX_SO_RES_INIT_ID + 3
#define IDS_SOCK_INVAL			HX_SO_RES_INIT_ID + 4
#define IDS_SOCK_MFILE			HX_SO_RES_INIT_ID + 5
#define IDS_SOCK_WOULDBLOCK		HX_SO_RES_INIT_ID + 6
#define IDS_SOCK_INPROGRESS		HX_SO_RES_INIT_ID + 7
#define IDS_SOCK_ALREADY		HX_SO_RES_INIT_ID + 8
#define IDS_SOCK_NOTSOCK		HX_SO_RES_INIT_ID + 9
#define IDS_SOCK_DESTADDRREQ		HX_SO_RES_INIT_ID + 10
#define IDS_SOCK_MSGSIZE		HX_SO_RES_INIT_ID + 11
#define IDS_SOCK_PROTOTYPE		HX_SO_RES_INIT_ID + 12
#define IDS_SOCK_NOPROTOOPT		HX_SO_RES_INIT_ID + 13
#define IDS_SOCK_PROTONOSUPPORT		HX_SO_RES_INIT_ID + 14
#define IDS_SOCK_SOCKTNOSUPPORT		HX_SO_RES_INIT_ID + 15
#define IDS_SOCK_OPNOTSUPP		HX_SO_RES_INIT_ID + 16
#define IDS_SOCK_PFNOSUPPORT		HX_SO_RES_INIT_ID + 17
#define IDS_SOCK_AFNOSUPPORT		HX_SO_RES_INIT_ID + 18
#define IDS_SOCK_ADDRINUSE		HX_SO_RES_INIT_ID + 19
#define IDS_SOCK_ADDRNOTAVAIL		HX_SO_RES_INIT_ID + 20
#define IDS_SOCK_NETDOWN		HX_SO_RES_INIT_ID + 21
#define IDS_SOCK_NETUNREACH		HX_SO_RES_INIT_ID + 22
#define IDS_SOCK_NETRESET		HX_SO_RES_INIT_ID + 23
#define IDS_SOCK_CONNABORTED		HX_SO_RES_INIT_ID + 24
#define IDS_SOCK_CONNRESET		HX_SO_RES_INIT_ID + 25
#define IDS_SOCK_NOBUFS			HX_SO_RES_INIT_ID + 26
#define IDS_SOCK_ISCONN			HX_SO_RES_INIT_ID + 27
#define IDS_SOCK_NOTCONN		HX_SO_RES_INIT_ID + 28
#define IDS_SOCK_SHUTDOWN		HX_SO_RES_INIT_ID + 29
#define IDS_SOCK_TOOMANYREFS		HX_SO_RES_INIT_ID + 30
#define IDS_SOCK_TIMEDOUT		HX_SO_RES_INIT_ID + 31
#define IDS_SOCK_CONNREFUSED		HX_SO_RES_INIT_ID + 32
#define IDS_SOCK_LOOP			HX_SO_RES_INIT_ID + 33
#define IDS_SOCK_NAMETOOLONG		HX_SO_RES_INIT_ID + 34
#define IDS_SOCK_HOSTDOWN		HX_SO_RES_INIT_ID + 35
#define IDS_SOCK_HOSTUNREACH		HX_SO_RES_INIT_ID + 36
#define IDS_SOCK_PIPE			HX_SO_RES_INIT_ID + 37
#define IDS_SOCK_ENDSTREAM		HX_SO_RES_INIT_ID + 38
#define IDS_SOCK_BUFFERED		HX_SO_RES_INIT_ID + 39
#define IDS_SOCK_NOTEMPTY		HX_SO_RES_INIT_ID + 40
#define IDS_SOCK_PROCLIM		HX_SO_RES_INIT_ID + 41
#define IDS_SOCK_USERS			HX_SO_RES_INIT_ID + 42
#define IDS_SOCK_DQUOT			HX_SO_RES_INIT_ID + 43
#define IDS_SOCK_STALE			HX_SO_RES_INIT_ID + 44
#define IDS_SOCK_REMOTE			HX_SO_RES_INIT_ID + 45
#define IDS_SOCK_SYSNOTREADY		HX_SO_RES_INIT_ID + 46
#define IDS_SOCK_VERNOTSUPPORTED	HX_SO_RES_INIT_ID + 47
#define IDS_SOCK_NOTINITIALISED		HX_SO_RES_INIT_ID + 48
#define IDS_SOCK_DISCON			HX_SO_RES_INIT_ID + 49
#define IDS_SOCK_NOMORE			HX_SO_RES_INIT_ID + 50
#define IDS_SOCK_CANCELLED		HX_SO_RES_INIT_ID + 51
#define IDS_SOCK_INVALIDPROCTABLE 	HX_SO_RES_INIT_ID + 52
#define IDS_SOCK_INVALIDPROVIDER 	HX_SO_RES_INIT_ID + 53
#define IDS_SOCK_PROVIDERFAILEDINIT	HX_SO_RES_INIT_ID + 54
#define IDS_SOCK_SYSCALLFAILURE		HX_SO_RES_INIT_ID + 55
#define IDS_SOCK_SERVICE_NOT_FOUND	HX_SO_RES_INIT_ID + 56
#define IDS_SOCK_TYPE_NOT_FOUND		HX_SO_RES_INIT_ID + 57
#define IDS_SOCK_E_NO_MORE		HX_SO_RES_INIT_ID + 58
#define IDS_SOCK_E_CANCELLED		HX_SO_RES_INIT_ID + 59
#define IDS_SOCK_REFUSED		HX_SO_RES_INIT_ID + 60
#define IDS_SOCK_HOST_NOT_FOUND		HX_SO_RES_INIT_ID + 61
#define IDS_SOCK_TRY_AGAIN		HX_SO_RES_INIT_ID + 62
#define IDS_SOCK_NO_RECOVERY		HX_SO_RES_INIT_ID + 63
#define IDS_SOCK_NO_DATA		HX_SO_RES_INIT_ID + 64

#endif
