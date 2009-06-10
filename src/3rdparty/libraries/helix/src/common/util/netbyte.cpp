/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: netbyte.cpp,v 1.20 2009/01/19 22:45:43 sfu Exp $
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
#include "hlxclib/ctype.h"
#include "hlxclib/sys/socket.h" // for AF_INET6, struct sockaddr, etc.
#include "hlxclib/limits.h"
#include "hlxclib/stdlib.h"
#include "safestring.h"
#include "netbyte.h"
#include "hxtypes.h"
#include "ihxpckts.h"
#include "hxbuffer.h"

#ifdef _MACINTOSH
#include "hxstrutl.h" // for isascii definition
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif

#if defined(HELIX_CONFIG_NOSTATICS)
# include "globals/hxglobals.h"
#endif

#if defined(HELIX_CONFIG_NOSTATICS)
# define STATIC_CHAR_ARRAY(name, length) \
    static const char _##name = '\0'; \
    char*& str = (char*&)HXGlobalCharArray::Get(&_##name, length, "" );
#else    
# define STATIC_CHAR_ARRAY(name, length) static char name[length];
#endif /* HELIX_CONFIG_NOSTATICS */



#ifndef NET_ENDIAN

NetWord WToNet(HostWord wHost) {
		/* Hopefully, a portable implementation
		** (although it's unnecessary labor on a
		** non-swapped machine) 
		*/
		NetWord wNet;
		unsigned char *uc = (unsigned char *)&wNet;
		*uc++ = (unsigned char) ((wHost & 0xFF00) >> 8);
		*uc++ = (unsigned char) (wHost & 0x00FF);
		return wNet;
} 	
HostWord WToHost(NetWord wNet) {
		HostWord wHost;
		unsigned char *uc = (unsigned char *)&wNet;
		wHost = (*uc++) << 8;
		wHost += (*uc++);
		return wHost;
}
NetDWord DwToNet(HostDWord dwHost) {
		NetDWord dwNet;
		unsigned char *uc = (unsigned char *)&dwNet;
		*uc++ = (unsigned char) ((dwHost & 0xFF000000L) >> 24);
		*uc++ = (unsigned char) ((dwHost & 0x00FF0000L) >> 16);
		*uc++ = (unsigned char) ((dwHost & 0x0000FF00L) >> 8);
		*uc++ = (unsigned char) (dwHost & 0x000000FFL);
		return dwNet;
} 	
HostDWord DwToHost(NetDWord dwNet) {
		HostDWord dwHost;
		unsigned char *uc = (unsigned char *)&dwNet;
		dwHost = ((unsigned long)(*uc++)) << 24;
		dwHost += ((unsigned long)(*uc++)) << 16;
		dwHost += ((unsigned long)(*uc++)) << 8;
		dwHost += (unsigned long)(*uc++);
		return dwHost;
}

#endif

int 
TestBigEndian(void)
{
    unsigned long test = 0x000000FF;
    char *temp = (char *)&test;

    /* on big endian machines, the first byte should be 0x00, on little endian 0xFF.*/

    return (temp[0] == 0);
}

/*  Converts 2 byte ints from big endian to little endian or vice versa */
void 
SwapWordBytes(HostWord *data, int numInts)
{
    INT32     i;
    INT16  temp;
    char    *temp1, *temp2;

    /* temp2 points to our temporary int that will hold the swapped bytes */
    temp2 = (char *) &temp;

    for(i = 0; i < numInts; i++)
    {
	temp1 = (char *) &data[i];

        temp2[0] = temp1[1];
	temp2[1] = temp1[0];

	data[i] = temp;
    }
}

/*  Converts 4 byte ints from big endian to little endian or vice versa */
void 
SwapDWordBytes(HostDWord *data, int numInts)
{
    INT32     i,temp;
    char    *temp1, *temp2;

    /* temp2 points to our temporary int that will hold the swapped bytes */
    temp2 = (char *) &temp;

    for(i = 0; i < numInts; i++)
    {
	temp1 = (char *) &data[i];

        temp2[0] = temp1[3];
        temp2[1] = temp1[2];
        temp2[2] = temp1[1];
	temp2[3] = temp1[0];

	data[i] = temp;
    }
}

/*
 * IP4 domain addresses will only be recognized if passed as
 * xxx.xxx.xxx.xxx where xxx is a value 0 to 255.
 */
int
IsNumericAddr(const char* addrStr, UINT32 size)
{
    int numericAddr = 0;
    int dotCount = 0;
    const char* ptr = NULL;
    int len = 0;
    int rc = 0;

    if (size == 0 || addrStr == NULL)
    {
        return 0;
    }

    len = size;
    ptr = addrStr;

    do
    {
        if (!dotCount && !isdigit(*ptr) && !numericAddr)
        {
            return rc; // First wasn't a digit.
        }
        else if (isdigit(*ptr))
        {
            numericAddr = 1; // A digit.
        }
        else if (numericAddr && *ptr == '.')
        {
            dotCount++;
            numericAddr = 0; // Now we need a digit again.
        }
        else if (dotCount && isdigit(*ptr))
        {
            numericAddr = 1; // Got digit after dot.
        }
        else
        {
            return rc; // Not a digit and not '.'
        }
        ptr++;
        len--;
    }
    while (*ptr && len);

    rc = (dotCount == 3) ? numericAddr : 0; // Must have 3 dots

    // If we have xxx.xxx.xxx.xxx, test that xxx doesn't exceed 255
    if (rc)
    {
        char val[4]; // Buffer for possible value
        char* tmp[5]; // Pointer to each segment
        int i;

        tmp[0] = (char*) addrStr;
        tmp[1] = strchr(tmp[0], '.') + 1;
        tmp[2] = strchr(tmp[1], '.') + 1;
        tmp[3] = strchr(tmp[2], '.') + 1;
        tmp[4] = tmp[0] + size + 1;

        for (i = 0; i < 4; i++)
        {
            memset(val, 0, sizeof(val));
            len = tmp[i + 1] - tmp[i] - 1;
            if (len > sizeof(val) - 1)
            {
                return (0);  // Larger than 3 digits
            }
            strncpy(val, tmp[i], len);

            if (atoi(val) > UCHAR_MAX)
            {
                return 0;  // Larger than 255 or 0xff
            }
        }
    }
    return rc;
}

void
NetLongToAscii(UINT32 addr, IHXBuffer* pAddrBuf)
{
    char str[16]; /* Flawfinder: ignore */  // max addr string length + NULL -- "255.255.255.255"
    char* ptr = (char *)str;
    char* prevStrEnd = 0;

    SafeSprintf(ptr, sizeof(str), "%lu", ((addr&0xff000000) >> 24));
    ptr = (char *)memchr(str, 0, 16);
    prevStrEnd = ptr;
    SafeSprintf(ptr, sizeof(str)-(ptr-str), ".%lu", ((addr&0x00ff0000) >> 16));
    ptr = (char *)memchr(prevStrEnd, 0, 16);
    prevStrEnd = ptr;
    SafeSprintf(ptr, sizeof(str)-(ptr-str), ".%lu", ((addr&0x0000ff00) >> 8));
    ptr = (char *)memchr(prevStrEnd, 0, 16);
    SafeSprintf(ptr, sizeof(str)-(ptr-str), ".%lu", (addr&0x000000ff));

    pAddrBuf->Set((BYTE *)str, strlen(str)+1);
}

// takes the addr in HOST-byte order!!!
// its too late in the cycle to make this fundamental a change
// so we r keeping it the same
char*
HXInetNtoa(UINT32 addr)
{
    STATIC_CHAR_ARRAY(str, 16); /* Flawfinder: ignore */ // max addr string length + NULL -- "255.255.255.255"
    char* ptr = (char *)str;
    char* prevStrEnd = 0;

    SafeSprintf(ptr, sizeof(str), "%lu", ((addr&0xff000000) >> 24));
    ptr = (char *)memchr(str, 0, 16);
    prevStrEnd = ptr;
    SafeSprintf(ptr, sizeof(str)-(ptr-str), ".%lu", ((addr&0x00ff0000) >> 16));
    ptr = (char *)memchr(prevStrEnd, 0, 16);
    prevStrEnd = ptr;
    SafeSprintf(ptr, sizeof(str)-(ptr-str), ".%lu", ((addr&0x0000ff00) >> 8));
    ptr = (char *)memchr(prevStrEnd, 0, 16);
    SafeSprintf(ptr, sizeof(str)-(ptr-str), ".%lu", (addr&0x000000ff));

    return str;
}

/*
* ++Copyright++ 1983, 1990, 1993
* -
* Copyright (c) 1983, 1990, 1993
*    The Regents of the University of California.  All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. All advertising materials mentioning features or use of this software
*    must display the following acknowledgement:
* This product includes software developed by the University of
* California, Berkeley and its contributors.
* 4. Neither the name of the University nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
* -
* Portions Copyright (c) 1993 by Digital Equipment Corporation.
* 
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies, and that
* the name of Digital Equipment Corporation not be used in advertising or
* publicity pertaining to distribution of the document or software without
* specific, written prior permission.
* 
* THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
* WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
* CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
* DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
* PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
* ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
* SOFTWARE.
* -
* --Copyright--
*/

/*
* Ascii internet address interpretation routine.
* The value returned is in network order.
* Check whether "cp" is a valid ascii representation
* of an Internet address and convert to a binary address.
* Returns 1 if the address is valid, 0 if not.
* This replaces inet_addr, the return value from which
* cannot distinguish between failure and a local broadcast address.
*/
UINT32
HXinet_addr(const char* cp)
{
    UINT32 val = 0;
    int base = 0, n = 0;
    char c;
    UINT parts[4];
    UINT *pp = parts;

    c = *cp;
    for (;;) 
    {
	/*
	* Collect number up to ``.''.
	* Values are specified as for C:
	* 0x=hex, 0=octal, isdigit=decimal.
	*/
	if (!isdigit(c))
	    return (INADDR_NONE);

	val = 0; base = 10;
	if (c == '0') 
	{
	    c = *++cp;
	    if (c == 'x' || c == 'X')
	    {
		base = 16, c = *++cp;
	    }
	    else
	    {
		base = 8;
	    }
	}
	
	for (;;) 
	{
	    if (isascii(c) && isdigit(c)) 
	    {
		val = (val * base) + (c - '0');
		c = *++cp;
	    } 
	    else if (base == 16 && isascii(c) && isxdigit(c)) 
	    {
		val = (val << 4) | (c + 10 - (islower(c) ? 'a' : 'A'));
		c = *++cp;
	    } 
	    else
	    {
		break;
	    }
	}

	if (c == '.') 
	{
	    /*
	    * Internet format:
	    * a.b.c.d
	    * a.b.c (with c treated as 16 bits)
	    * a.b (with b treated as 24 bits)
	    */
	    if (pp >= parts + 3)
	    {
		return (INADDR_NONE);
	    }

	    *pp++ = val;
	    c = *++cp;
	} 
	else
	{
	    break;
	}
    }

    /*
    * Check for trailing characters.
    */
    if (c != '\0' && (!isascii(c) || !isspace(c)))
    {
	return (INADDR_NONE);
    }

    /*
    * Concoct the address according to
    * the number of parts specified.
    */
    n = pp - parts + 1;

    switch (n) 
    {
	case 0:
	    return (INADDR_NONE); /* initial nondigit */

	case 1: /* a -- 32 bits */
	    break;

	case 2: /* a.b -- 8.24 bits */
	    if (val > 0xffffff)
		return (INADDR_NONE);
	    val |= parts[0] << 24;
	    break;

	case 3: /* a.b.c -- 8.8.16 bits */
	    if (val > 0xffff)
		return (INADDR_NONE);
	    val |= (parts[0] << 24) | (parts[1] << 16);
	    break;

	case 4: /* a.b.c.d -- 8.8.8.8 bits */
	    if (val > 0xff)
		return (INADDR_NONE);
	    val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
	    break;
    }

    return DwToNet(val);
}

// to store actual netbyte order of bytes in a string
// on a little endian machine basically swap the bytes
char*
NetLongAddrToDecimalStr(ULONG32 ulAddr)
{
    STATIC_CHAR_ARRAY(str, 11);/* Flawfinder: ignore */ // max decimal digits (4 Gig) for a 4-byte number

    unsigned long test = 0x000000FF;
    char *temp = (char *)&test;
    if (temp[0] != 0x0) // big-endian
    {
	ULONG32 addr = 0;
	char* temp1 = (char *)&addr;
	temp = (char *)&ulAddr;
	temp1[0] = temp[3];
	temp1[1] = temp[2];
	temp1[2] = temp[1];
	temp1[3] = temp[0];
	SafeSprintf(str, sizeof(str), "%lu", (unsigned long)addr);
    }
    else
    {
	SafeSprintf(str, sizeof(str), "%lu", (unsigned long)ulAddr);
    }
    return str;
}

char*
NetLongToAsciiStr(UINT32 addr)
{
    STATIC_CHAR_ARRAY(str, 16);/* Flawfinder: ignore */ // max addr string length + NULL -- "255.255.255.255"
    UCHAR* tmp = (UCHAR *)&addr;
    char* ptr = (char *)str;
    char* prevStrEnd = 0;

    SafeSprintf(ptr, sizeof(str), "%u", tmp[0]);
    ptr = (char *)memchr(str, 0, 16);
    prevStrEnd = ptr;
    SafeSprintf(ptr, sizeof(str)-(ptr-str), ".%u", tmp[1]);
    ptr = (char *)memchr(str, 0, 16);
    prevStrEnd = ptr;
    SafeSprintf(ptr, sizeof(str)-(ptr-str), ".%u", tmp[2]);
    ptr = (char *)memchr(str, 0, 16);
    prevStrEnd = ptr;
    SafeSprintf(ptr, sizeof(str)-(ptr-str), ".%u", tmp[3]);

    return str;
}

UINT32
CalculateIPv4Prefix(UINT8* pin)
{
    HXBOOL   bDone = FALSE;
    UINT32 i, j = 0;
    UINT32 ulResult = 0;
    
    if (pin)
    {
        for (i = 0; i < 4; i++)
        {
            if (255 == pin[i])
            {
                ulResult+=8;
            }
            else
            {
                for (j = 0; j < 8; j++)
                {
                    if ((0x80 >> j) & pin[i])
                    {
                        ulResult++;
                    }
                    else
                    {
                        bDone = TRUE;
                        break;
                    }
                }
            }

            if (bDone)
            {
                break;
            }
        }
    }

    return ulResult;
}

int
NetAddressToAscii(const struct sockaddr* pAddr, IHXBuffer* pBuffer)
{
    int result = -1;

    if (pBuffer && pAddr)
    {
        char szAddress[40] = {0}; // max. IPv6 address length

        struct sockaddr* pa = (struct sockaddr*)pAddr;
        switch (pa->sa_family)
        {
        case AF_INET:
            {
                const struct sockaddr_in* psa = reinterpret_cast<const struct sockaddr_in*>(pa);
                const UINT8* pin = (const UINT8*)&psa->sin_addr;

                sprintf(szAddress, "%u.%u.%u.%u", 
                        (unsigned int)pin[0], (unsigned int)pin[1], 
                        (unsigned int)pin[2], (unsigned int)pin[3]);
                pBuffer->Set((const unsigned char*)szAddress, strlen(szAddress) + 1);
        
                result = 0;
            }
            break;

#if !defined(_SYMBIAN) && !defined(_WINCE) &&!defined(_BREW) && !defined(ANDROID)
        case AF_INET6:
            {
                const struct sockaddr_in6* psa = reinterpret_cast<const struct sockaddr_in6*>(pa);
                const UINT16* pin = (const UINT16*)&psa->sin6_addr;

                sprintf(szAddress, "%hx:%hx:%hx:%hx:%hx:%hx:%hx:%hx", 
                        WToHost(pin[0]), WToHost(pin[1]), WToHost(pin[2]), WToHost(pin[3]), 
                        WToHost(pin[4]), WToHost(pin[5]), WToHost(pin[6]), WToHost(pin[7]));
                pBuffer->Set((const unsigned char*)szAddress, strlen(szAddress) + 1);

                result = 0;
            }
            break;
#endif /* !defined(_SYMBIAN) */

        default:
            HX_ASSERT(FALSE);
            break;
        }
    }

    return result;
}
