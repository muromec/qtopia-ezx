/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: nettypes.h,v 1.2 2005/03/22 19:59:44 liam_murray Exp $
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

#ifndef NETTYPES_H__
#define NETTYPES_H__

#include <in_sock.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h> // memcmp
#include "hxtypes.h"

// Provides posix data structures missing from standard Symbian headers. Even though
// Symbian does not provide a posix socket API, these are useful for address representation
// in generic cross-platfrom code.
//

#if !defined(AF_INET6)
#define AF_INET6 (AF_INET + 1)
#endif //AF_INET6

#if !defined(INET6_ADDRSTRLEN)
#define INET6_ADDRSTRLEN KInetAddrMaxBits
#endif //INET6_ADDRSTRLEN

typedef UINT32 in_addr_t;
typedef UINT16 sa_family_t;

struct sockaddr_un 
{ 
    sa_family_t sun_family; 
    char sun_path[128]; 
};

struct in6_addr 
{
    union 
    {
        UINT8 Byte[16];
        UINT16 Word[8];
    } u;
};

#define s6_bytes    u.Byte
#define s6_words    u.Word
#define s6_addr     s6_bytes

struct sockaddr_in6 
{
    sa_family_t sin6_family;
    UINT16      sin6_port;
    UINT32      sin6_flowinfo;
    in6_addr    sin6_addr;
    UINT32      sin6_scope_id;
};


// see RFC 2553 3.10
//
#define SS_MAXSIZE 128
#define SS_ALIGNSIZE (sizeof(INT64))
#define SS_PAD1SIZE (SS_ALIGNSIZE - sizeof (sa_family_t))
#define SS_PAD2SIZE (SS_MAXSIZE - (sizeof (sa_family_t) + SS_PAD1SIZE + SS_ALIGNSIZE))

struct sockaddr_storage 
{
    sa_family_t     ss_family;
    UINT8           ss_pad1__[SS_PAD1SIZE];
    UINT64          ss_align__;
    UINT8           ss_pad2__[SS_PAD2SIZE];
};


// IN4 macros
//
#define IS_IN_MULTICAST(i)  IN_CLASSD(i)

// IN6 macros
//
inline 
HXBOOL 
IN6_ARE_ADDR_EQUAL(const in6_addr* a, const in6_addr* b)
{
    return (memcmp(a, b, sizeof(in6_addr)) == 0);
}

inline 
HXBOOL 
IN6_IS_ADDR_UNSPECIFIED(const in6_addr* a)
{
    return ((a->s6_words[0] == 0) &&
            (a->s6_words[1] == 0) &&
            (a->s6_words[2] == 0) &&
            (a->s6_words[3] == 0) &&
            (a->s6_words[4] == 0) &&
            (a->s6_words[5] == 0) &&
            (a->s6_words[6] == 0) &&
            (a->s6_words[7] == 0));
}

inline 
HXBOOL 
IN6_IS_ADDR_LOOPBACK(const in6_addr* a)
{
    return ((a->s6_words[0] == 0) &&
            (a->s6_words[1] == 0) &&
            (a->s6_words[2] == 0) &&
            (a->s6_words[3] == 0) &&
            (a->s6_words[4] == 0) &&
            (a->s6_words[5] == 0) &&
            (a->s6_words[6] == 0) &&
            (a->s6_words[7] == 0x0100));
}

inline 
HXBOOL
IN6_IS_ADDR_MULTICAST(const in6_addr* a)
{
    return (a->s6_bytes[0] == 0xff);
}

inline 
HXBOOL
IN6_IS_ADDR_LINKLOCAL(const in6_addr* a)
{
    return ((a->s6_bytes[0] == 0xfe) &&
            ((a->s6_bytes[1] & 0xc0) == 0x80));
}

inline 
HXBOOL
IN6_IS_ADDR_SITELOCAL(const in6_addr* a)
{
    return ((a->s6_bytes[0] == 0xfe) &&
            ((a->s6_bytes[1] & 0xc0) == 0xc0));
}

inline 
HXBOOL
IN6_IS_ADDR_V4MAPPED(const in6_addr* a)
{
    return ((a->s6_words[0] == 0) &&
            (a->s6_words[1] == 0) &&
            (a->s6_words[2] == 0) &&
            (a->s6_words[3] == 0) &&
            (a->s6_words[4] == 0) &&
            (a->s6_words[5] == 0xffff));
}

inline 
HXBOOL
IN6_IS_ADDR_V4COMPAT(const in6_addr* a)
{
    return ((a->s6_words[0] == 0) &&
            (a->s6_words[1] == 0) &&
            (a->s6_words[2] == 0) &&
            (a->s6_words[3] == 0) &&
            (a->s6_words[4] == 0) &&
            (a->s6_words[5] == 0) &&
            !((a->s6_words[6] == 0) &&
              (a->s6_addr[14] == 0) &&
             ((a->s6_addr[15] == 0) || (a->s6_addr[15] == 1))));
}

inline 
HXBOOL
IN6ADDR_ISANY(const sockaddr_in6* a)
{
    return ((a->sin6_family == AF_INET6) &&
            IN6_IS_ADDR_UNSPECIFIED(&a->sin6_addr));
}

inline 
HXBOOL
IN6ADDR_ISLOOPBACK(const sockaddr_in6* a)
{
    return ((a->sin6_family == AF_INET6) &&
            IN6_IS_ADDR_LOOPBACK(&a->sin6_addr));
}



#endif /* NETTYPES_H__ */
