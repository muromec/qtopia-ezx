/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: common_dispatch.h,v 1.7 2004/08/18 22:56:58 tmarshall Exp $
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

#ifndef _COMMON_DISPATCH_H
#define _COMMON_DISPATCH_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "debug.h"

#include "hxnet.h"
#include "sockimp.h"
#include "servsockimp.h"

#if !defined _WINDOWS
class CloseFDCallback : public SimpleCallback
{
public:
    void            func(Process *proc);
    int             fd;
#if defined _SUN || defined _SOLARIS
    int             received_procnum;
#endif
                    ~CloseFDCallback() {};
};

inline void
CloseFDCallback::func(Process* proc)
{
    /* Descriptor was accounted for in disconnect/reconnect */
    if (close(fd) < 0)
        DPRINTF(D_INFO, ("close(%d) -- %s\n", fd, strerror(errno)));
#if defined PAULM_SOCKTIMING
    g_SocketTimer.Remove(fd);
#endif
    delete this;
}

#endif /* ! defined _WINDOWS */

class SockDispatchCallback : public SimpleCallback, public BaseCallback
{
public:
    virtual void func(Process* proc) = 0;
    STDMETHOD(Func)(THIS) PURE;

    CHXServSocket*      m_pSock;

#if !defined _WIN32
    FDPassSocket* fdp;
    int defunct;
    int fd;
#if defined _SUN || defined _SOLARIS
    int received_procnum;
#endif

#endif

protected:
    ~SockDispatchCallback()
    {
        if (m_pSock)
        {
            m_pSock->Release();
            m_pSock = 0;
        }
    };

};

#endif /* _COMMON_DISPATCH_H */
