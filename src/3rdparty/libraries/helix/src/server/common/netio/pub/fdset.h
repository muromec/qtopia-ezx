/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fdset.h,v 1.3 2004/06/18 19:31:48 tmarshall Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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

#ifndef _FDSET_H_
#define _FDSET_H_

#include "asyncio.h"
#ifdef _AIX
#include <sys/select.h>
#endif

#ifndef MAX_FDS
#define MAX_FDS 2048
#endif /* MAX_FDS */

#ifndef MAX_NFDBITS
#define MAX_NFDBITS 32
#endif

class FD_set
{
public:
    int         count;
    int         high;
    fd_mask*    bits;
    fd_set*     set;

                FD_set();
                ~FD_set();
    void        destroy();
    void        init(int limit = MAX_FDS);
    void        add(int fd);
    void        remove(int fd);
    void        add(AsyncIO* aio);
    void        remove(AsyncIO* aio);
    void        zero();
    int         members(int* list);
    void        callback(int* list);
    void        sub(FD_set* fdset1);
    void        copy(FD_set* target);
};

inline void FD_set::destroy()
{
    delete[] bits;
    bits = 0;
}

inline FD_set::FD_set() {
    count = 0;
    high = -1;
    bits = 0;
}

inline FD_set::~FD_set() {
    destroy();
}

inline void FD_set::init(int limit)
{
    int i;

    /*
     * Allocate atleast MAX_FDS set as some Unix select()
     * implementations may corrupt data after shorter arrays.
     */
    if (limit < MAX_FDS)
    {
        limit = MAX_FDS;
    }
    count = (limit + MAX_NFDBITS - 1)/MAX_NFDBITS;
    bits = new fd_mask[count];
    for (i = 0; i < count; i++)
    {
        bits[i] = 0;
    }
    set = (fd_set*)bits;
}

inline void FD_set::add(int fd)
{
    if (fd < 0)
    {
        return;
    }
    if (high < fd)
    {
        high = fd;
    }
    bits[fd/MAX_NFDBITS] |= 1 << (fd % MAX_NFDBITS);
}

inline void FD_set::remove(int fd)
{
    if (fd < 0)
    {
        return;
    }
    bits[fd/MAX_NFDBITS] &= ~(1 << (fd % MAX_NFDBITS));
}

inline void FD_set::add(AsyncIO* io)
{
    add(io->fd());
}

inline void FD_set::remove(AsyncIO* io)
{
    remove(io->fd());
}

inline void FD_set::zero()
{
    high = -1;
    memset(bits, 0, count*sizeof(fd_mask));
}

inline int FD_set::members(int* rlist)
{
    int *l = rlist;
    for (int i = 0; i < count; i++)
    {
        fd_mask m;
        unsigned j;
        if (bits[i])
        {
            for (j = 0, m = 1; j < MAX_NFDBITS; j++, m <<= 1)
            {
                if (bits[i] & m)
                {
                    *l++ = i*MAX_NFDBITS+j;
                }
            }
        }
    }
    return l - rlist;
}

inline void FD_set::copy(FD_set* target)
{
    target->high = high;
    memcpy(target->bits, bits, count*sizeof(fd_mask));
}

inline void FD_set::sub(FD_set* fdset1)
{
    for (int i = 0; i < count; i++)
    {
        bits[i] &= ~fdset1->bits[i];
    }
}

#endif/*_FDSET_H_*/
