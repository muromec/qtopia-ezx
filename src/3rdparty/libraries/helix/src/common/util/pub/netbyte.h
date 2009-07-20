/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: netbyte.h,v 1.9 2007/07/06 20:39:23 jfinnecy Exp $
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

#ifndef _MSDOS
#ifndef _WIN32
#ifndef _UNIX
#ifndef _SYMBIAN
#ifndef REAL_MEDIA_FILE_SERVER_PORT
#pragma once
#endif
#endif
#endif 
#endif
#endif
 
#ifndef NETBYTE_H
#define NETBYTE_H 1

#include "hxtypes.h"

#if defined (_UNIX)
#include <unistd.h>
#include <netinet/in.h>
#endif

struct IHXBuffer;

typedef UINT16	NetWord, HostWord;
typedef UINT32	NetDWord, HostDWord;
#ifdef __cplusplus
	#ifndef NET_ENDIAN
		extern "C" {
		extern NetWord	WToNet(HostWord wHost);
		extern HostWord WToHost(NetWord wNet);
		extern NetDWord DwToNet(HostDWord dwHost);
		extern HostDWord DwToHost(NetDWord dwNet);
		}
	#else
		inline NetWord WToNet(HostWord wHost) {return (wHost);};
		inline HostWord WToHost(NetWord wNet) {return (wNet);};
		inline NetDWord DwToNet(HostDWord dwHost) {return (dwHost);};
		inline HostDWord DwToHost(NetDWord dwNet) {return (dwNet);};
	#endif
#else
	#ifndef NET_ENDIAN
		extern NetWord WToNet(HostWord wHost);
		extern HostWord WToHost(NetWord wNet);
		extern NetDWord DwToNet(HostDWord dwHost);
		extern HostDWord DwToHost(NetDWord dwNet);
	#else
		#define WToNet(wHost)	((NetWord) (wHost))
		#define WToHost(wNet)	((HostWord) (wNet ))
		#define DwToNet(dwHost) ((NetDWord) (dwHost))
		#define DwToHost(dwNet) ((HostDWord) (dwNet	))
	#endif
#endif

#ifdef __cplusplus
extern "C" 
{
int TestBigEndian(void);
}
#endif // __cplusplus


/*  Converts 2 byte ints from big endian to little endian or vice versa */
void SwapWordBytes(HostWord *data, int numInts);

/*  Converts 4 byte ints from big endian to little endian or vice versa */
void SwapDWordBytes(HostDWord *data, int numInts);

/*
 * Test if a given IP address is string or numeric. 
 * Return 1 is NUMERIC, and 0 if STRING
 * e.g.
 * 177.345.3.54 ---> numeric address (all digits and 3 dots '.')
 * foobar.prognet.com ---> string address
 * 2001.space.odessey.com --> string address
 */
int IsNumericAddr(const char* addr, UINT32 len);
/*
 * On IRIX the inet_ntoa() method is broken, so this method is a
 * for all platforms. 
 * It takes a HOST byte ordered IP address and an empty IHXBuffer 
 * into which it stuffs a string of the numeric for of the IP addr
 * aaa.bbb.ccc.ddd
 *
 * SHOULD take a network byte orderd address, but it was incorrectly
 * developed and all over the server we rely on the fact that
 * NetLongToAscii() and and HXInetNtoa() accept addresses in the HOST
 * byte order. and we are too late in the release cycle to fix this
 * problem.
 *
 * XXXAAK -- for now it will only create full IP addresses a.b.c.d
 * and not of the type b.c.d or c.d
 */
void NetLongToAscii(UINT32 addr, IHXBuffer* addrBuf);

char* HXInetNtoa(UINT32 addr);

UINT32 HXinet_addr(const char* cp);

// mainly for sprintf'ing a NETBYTE order decimal string
char* NetLongAddrToDecimalStr(ULONG32 ulAddr);

// actually takes in a NET byte ordered address
char* NetLongToAsciiStr(UINT32 addr);

// calcuate the prefix(continuous 1s from the left most)
UINT32  CalculateIPv4Prefix(UINT8* pin);
// convert address stored in sockaddr to string
int     NetAddressToAscii(const struct sockaddr* pAddr, IHXBuffer* pBuffer);

#endif /* NETBYTE_H */
