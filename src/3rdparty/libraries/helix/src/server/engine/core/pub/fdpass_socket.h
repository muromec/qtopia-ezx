/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fdpass_socket.h,v 1.6 2005/02/17 01:34:12 atin Exp $
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

#ifndef _FDPASS_SOCKET_H_
#define _FDPASS_SOCKET_H_

#if !defined WIN32

#include "debug.h"
#include "passfd.h"
#include "proc.h"
#include "trace.h"
#include <stdio.h>

class FDPassSocket {
public:
                            FDPassSocket(int _s[2], int _p[2], Process* _proc);
    void                    SetSocket0(int _s)      { s[0] = _s;   }
    int                     GetSocket0()            { return s[0]; }
    int                     Send(int fd, int p);
    int                     Recv(int p);

private:
    int                     s[2], p[2];
    Process*                proc;
};


inline
FDPassSocket::FDPassSocket(int _s[2], int _p[2], Process* _proc)
            : proc(_proc)
{
    memcpy(s, _s, sizeof (int) * 2);
    memcpy(p, _p, sizeof (int) * 2);
}

/*
 *  Procnum is the process number that you are sending the descriptor to.
 */
inline int
FDPassSocket::Send(int fd, int procnum)
{
    DPRINTF(0x01000000, ("FDPS(%p)::Send(fd(%d), procnum(%d))\n",
        this, fd, procnum));
    int ss;

    if (procnum == p[0])
        ss = s[0];
    else if (procnum == p[1])
        ss = s[1];
    else
    {
        PANIC(("FDPassSocket::Send(): Send to unknown process\n"));
        return 0; // Shut up GCC
    }

    if (send_connection(ss, fd) < 0)
        return -1;
    else
    {
        close(fd);
        return 0;
    }
}


/*
 *  Procnum is *this* process's number
 */
inline int
FDPassSocket::Recv(int procnum)
{
    DPRINTF(0x01000000, ("FDPS(%p)::Recv(procnum(%d))\n", this, procnum));
    int ss;

    // XXXSMP We really don't *have to* check this
    if (procnum == p[0])
        ss = s[1];
    else if (procnum == p[1])
        ss = s[0];
    else
    {
        PANIC(("FDPassSocket::Recv(): Send to unknown process\n"));
        return 0; // Shut up GCC
    }

    int ret = recv_connection(ss);
    return ret;
}

#endif /* !WIN32 */

#endif /* _FDPASS_SOCKET_H_ */
