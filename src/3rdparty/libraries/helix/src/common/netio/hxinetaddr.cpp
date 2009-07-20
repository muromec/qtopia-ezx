/* ***** BEGIN LICENSE BLOCK *****
 * Version: RCSL 1.0/RPSL 1.0
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 *
 * The contents of this file, and the files included with this file, are
 * subject to the current version of the RealNetworks Public Source License
 * Version 1.0 (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the RealNetworks Community Source License Version 1.0
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl,
 * in which case the RCSL will apply. You may also obtain the license terms
 * directly from RealNetworks.  You may not use this file except in
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks
 * applicable to this file, the RCSL.  Please see the applicable RPSL or
 * RCSL for the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the portions
 * it created.
 *
 * This file, and the files included with this file, is distributed and made
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "hxtypes.h"
#include "nettypes.h"
#include "hxnet.h"
#include "hxinetaddr.h"
#include "netbyte.h"
#include "hxassert.h"
#include "hxbuffer.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif


static inline HXBOOL
isx(char c)
{
    return ( (c >= '0' && c <= '9') ||
             (c >= 'A' && c <= 'F') ||
             (c >= 'a' && c <= 'f') );
}

static inline HXBOOL
isd(char c)
{
    return (c >= '0' && c <= '9');
}

static inline int
x2n(char c)
{
    if (c >= '0' && c <= '9') return c-'0';
    if (c >= 'A' && c <= 'F') return c-'A'+10;
    if (c >= 'a' && c <= 'f') return c-'a'+10;
    return 0;
}

// The n2x routine is equivalent to this:
// tp += sprintf(tp, "%x", words[i]);
// However, this code is in the server's packet path, so using sprintf()
// here is a severe CPU penalty for the server.
// Note that pBuf needs to be at least 5 bytes to allow room for the ending null byte.
static inline int
n2x(UINT32 ulNum, char* pBuf)
{
    for (int j=3; j >= 0; --j)
    {
        unsigned int uNibble;
        if (j)
        {
            uNibble = (ulNum >> (4*j)) & 0xf;
        }
        else
        {
            uNibble = ulNum & 0xf;
        }
        unsigned char ch = (unsigned char)uNibble;
        HX_ASSERT((UINT32)ch <= 15);
        ch += (ch < 10 ? '0' : ('a' - 10));
        *pBuf++ = ch;
    }
    *pBuf = '\0';
    return 4;
}

static int
hx_inet_pton4(const char* s, UINT8* d)
{
    // NB: inet_pton() does not support BSD style partial address formats
    unsigned int elem, pos, n;
    elem = 0;
    while (*s)
    {
        n = pos = 0;
        while (isd(*s))
        {
            if (pos++ > 2)
            {
                return 0;
            }
            n = n*10 + (*s++ - '0');
        }
        if (pos == 0 || n > 255)
        {
            return 0;
        }
        d[elem] = n;
        if (elem < 3)
        {
            if (*s != '.')
            {
                return 0;
            }
            ++s;
        }
        if (elem++ > 3)
        {
            return 0;
        }
    }

    return (elem == 4) ? 1 : 0;
}

static int
hx_inet_pton6(const char* s, UINT16* d)
{
    unsigned int elem, pos, n;
    unsigned int celem = (~0U); // Compression element
    in_addr addr4;

    // Check for IPv4 literal and convert to V4MAPPED directly
    if (hx_inet_pton4(s, (UINT8*)&addr4) > 0)
    {
        memset(d, 0, 10);
        d[5] = 0xffff;
        memcpy(&d[6], &addr4, sizeof(addr4));
        return 1;
    }

    elem = 0;
    /*
     * We require the leading "::ffff:" for v4mapped addresses because our
     * hx_inet_pton handles "bare" IPv4 addresses.  If that were not so, we
     * would need to do a special check here.
     */
    if (*s == ':' && *(s+1) == ':')
    {
        celem = 0;
        s += 2;
    }
    while (*s)
    {
        n = pos = 0;
        if (hx_inet_pton4(s, (UINT8*)&addr4) > 0)
        {
            /*
             * Check for mixed v6/v4 notation (see RFC 3513 2.2 #3).  The
             * IPv4 portion must be at the end of the address.  I don't see
             * any requirement that the prefix is any particular value (eg.
             * "::" or "::ffff:") so don't try to check.
             */
            if ((celem == (~0U) && elem != 6) || (celem > 5 || elem > 6))
            {
                return 0;
            }
            memcpy(&d[elem], &addr4, sizeof(addr4));
            elem += 2;
            break;
        }
        else
        {
            while (isx(*s))
            {
                if (pos++ > 3)
                {
                    return 0;
                }
                n = (n << 4) + x2n(*s++);
            }
            if (pos == 0)
            {
                return 0;
            }
            d[elem] = hx_htons(n);
        }
        if (*s)
        {
            if (*s != ':')
            {
                return 0;
            }
            ++s;
            if (*s == ':')
            {
                if (celem != (~0U))
                {
                    return 0;
                }
                celem = elem+1;
                ++s;
            }
        }
        if (elem++ > 7)
        {
            return 0;
        }
    }
    if (celem != (~0U))
    {
        if (elem > celem)
        {
            memmove(&d[8-(elem-celem)], &d[celem],
                    (elem-celem)*sizeof(UINT16));
        }
        memset(&d[celem], 0, (8-elem)*sizeof(UINT16));
    }
    else if(elem != 8)
    {
        return 0;
    }
    return 1;
}

static const char*
hx_inet_ntop4(const void* s, char* d, UINT32 len)
{
    if (len < HX_ADDRSTRLEN_IN4)
    {
        return NULL;
    }

    const UINT8* p = (const UINT8*)s;
    sprintf(d, "%u.%u.%u.%u",
            (unsigned int)p[0], (unsigned int)p[1],
            (unsigned int)p[2], (unsigned int)p[3]);

    return d;
}

/*
 * This version of inet_ntop6 was originally written by Paul Vixie in 1996
 * and imported into various BSD projects.  The copyright is as follows:
 *
 * Copyright (c) 1996 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
static const char*
hx_inet_ntop6(const void* s, char* d, UINT32 len)
{
    char tmp[INET6_ADDRSTRLEN];
    char* tp;
    struct { int base, len; } best, cur;
    UINT16 words[sizeof(in6_addr)/2];
    int i;

    // Copy the input (bytewise) array into a wordwise array
    memset(words, 0, sizeof(words));
    for (i = 0; i < (int)sizeof(in6_addr)/2; i++)
    {
        words[i] = (*((UINT8*)s+i*2) << 8) | *((UINT8*)s+i*2+1);
    }

    // Find the longest run of 0x00's in source for :: shorthand
    best.base = cur.base = -1;
    for (i = 0; i < (int)sizeof(in6_addr)/2; i++)
    {
        if (words[i] == 0)
        {
            if (cur.base == -1)
            {
                cur.base = i;
                cur.len = 1;
            }
            else
            {
                cur.len++;
            }
        }
        else
        {
            if (cur.base != -1)
            {
                if (best.base == -1 || cur.len > best.len)
                {
                    best = cur;
                }
                cur.base = -1;
            }
        }
    }
    if (cur.base != -1)
    {
        if (best.base == -1 || cur.len > best.len)
        {
            best = cur;
        }
    }
    if (best.base != -1 && best.len < 2)
    {
        best.base = -1;
    }

    // Format the result
    tp = tmp;

    // Is this address an encapsulated IPv4?
    if ((best.base == 0 && best.len == 6) ||
        (best.base == 0 && best.len == 5 && words[5] == 0xffff))
    {
        if (!hx_inet_ntop4((UINT8*)s+12, tp, sizeof(tmp)))
        {
            return NULL;
        }
        tp += strlen(tp);
    }
    else
    {
        for (i = 0; i < (int)sizeof(in6_addr)/2; i++)
        {
            // Are we inside the best run of 0x00's?
            if (best.base != -1 && i >= best.base && i < (best.base + best.len))
            {
                if (i == best.base)
                {
                    *tp++ = ':';
                }
                continue;
            }

            // Are we following an initial run of 0x00s or any real hex?
            if (i != 0)
            {
                *tp++ = ':';
            }

            tp += n2x(words[i], tp);
        }

        // Was it a trailing run of 0x00's?
        if (best.base != -1 && (best.base + best.len) == (sizeof(in6_addr)/2))
        {
            *tp++ = ':';
        }
    }

    *tp++ = '\0';

    // Check for overflow, copy, and we're done.
    if ((size_t)(tp - tmp) > len)
    {
        return NULL;
    }
    strcpy(d, tmp);
    return d;
}


UINT32
hx_htonl(UINT32 n)
{
    return DwToNet(n);
}

UINT16
hx_htons(UINT16 n)
{
    return WToNet(n);
}

UINT32
hx_ntohl(UINT32 n)
{
    return DwToHost(n);
}

UINT16
hx_ntohs(UINT16 n)
{
    return WToHost(n);
}

int
hx_inet_pton(int af, const char* s, void* d)
{
    switch (af)
    {
    case AF_INET:
        return hx_inet_pton4(s, (UINT8*)d);
    case AF_INET6:
        return hx_inet_pton6(s, (UINT16*)d);
    }

    return -1; // Unsupported family
}

const char*
hx_inet_ntop(int af, const void* s, char* d, UINT32 len)
{
    switch (af)
    {
    case AF_INET:
        return hx_inet_ntop4(s, d, len);
    case AF_INET6:
        return hx_inet_ntop6(s, d, len);
    }
    return NULL; // Unsupported family
}


static HXBOOL
do_mask(UINT32 len, BYTE* p, UINT32 bits)
{
    if (bits > len*8)
    {
        return FALSE;
    }
    p += len-1;                 // Point to the last octet
    bits = len*8 - bits;        // Get number of bits to zero out
    while (bits >= 8)
    {
        *p-- = 0;
        bits -= 8;
    }
    if (bits != 0)
    {
        *p &= ~((1<<bits)-1);
    }
    return TRUE;
}

HXBOOL
hx_maskaddr4(in_addr* paddr, UINT32 bits)
{
    return do_mask(sizeof(in_addr), (BYTE*)&paddr->s_addr, bits);
}

HXBOOL
hx_maskaddr6(in6_addr* paddr, UINT32 bits)
{
    return do_mask(sizeof(in6_addr), (BYTE*)&paddr->s6_addr, bits);
}

void
hx_map4to6(const sockaddr* psa, sockaddr_in6* psa6)
{
    sockaddr_in* psa4 = (sockaddr_in*)psa;
    memset(psa6, 0, sizeof(sockaddr_in6));
    psa6->sin6_family = AF_INET6;
    psa6->sin6_port = psa4->sin_port;
    psa6->sin6_addr.s6_addr[10] = 0xff;
    psa6->sin6_addr.s6_addr[11] = 0xff;
    memcpy(&psa6->sin6_addr.s6_addr[12], &psa4->sin_addr.s_addr, 4);
}

HXBOOL
hx_map6to4(const sockaddr* psa, sockaddr_in* psa4)
{
    sockaddr_in6* psa6 = (sockaddr_in6*)psa;
    memset(psa4, 0, sizeof(sockaddr_in));
    if (IN6_IS_ADDR_V4MAPPED(&psa6->sin6_addr))
    {
        psa4->sin_family = AF_INET;
        psa4->sin_port = psa6->sin6_port;
        memcpy(&psa4->sin_addr.s_addr, &psa6->sin6_addr.s6_addr[12], 4);
        return TRUE;
    }
    return FALSE;
}
