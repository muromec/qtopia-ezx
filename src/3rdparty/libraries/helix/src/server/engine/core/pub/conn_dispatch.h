/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: conn_dispatch.h,v 1.14 2005/08/15 17:36:53 srobinson Exp $
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

#ifndef _CONN_DISPATCH_H_
#define _CONN_DISPATCH_H_

#include "hxtypes.h"
#include "platform_config.h"
#include "debug.h"
#include "streamer_info.h"
#include "_main.h"
#include "simple_callback.h"
#include "base_callback.h"
#include "dispatchq.h"
#include "hxslist.h"
#include "fdpass_socket.h"
#include "sockfd.h"
#include "sockio.h"
#include "tcpio.h"
#include "client.h"
#include "common_dispatch.h"

#if defined PAULM_SOCKTIMING
#include "sockettimer.h"

extern SocketTimer g_SocketTimer;
#endif

extern BOOL* g_pProcessStartInProgress;

class cd_SockDispatchCallback : public SockDispatchCallback
{
public:
    void func(Process* proc);
    STDMETHOD(Func)(THIS);
};


class ConnDispatch
{
public:
                        ConnDispatch(Process* _proc);
    int                 Send(CHXServSocket* pSock, int newproc);
    void                process();

    Process*            proc;
    CHXSimpleList       waiters;
};

inline
ConnDispatch::ConnDispatch(Process* _proc)
{
    proc = _proc;
}

inline HX_RESULT
cd_SockDispatchCallback::Func()
{
    return HXR_OK;
}

inline void
cd_SockDispatchCallback::func(Process* proc)
{
#if !defined WIN32
#ifdef SHARED_FD_SUPPORT
    if (!g_bSharedDescriptors)
#endif /* SHARED_FD_SUPPORT */
    {
        int newfd = fdp->Recv(proc->procnum());
        if (newfd < 0)
        {
            fprintf(stderr, "failed to recv fd(%d) in proc(%d)\n",
                newfd, proc->procnum());
            goto CloseOtherSide;
        }

        m_pSock->SetSock(newfd);
    }
#endif

    m_pSock->EnterProc(proc);

#if !defined WIN32
CloseOtherSide:
#endif

    HX_RELEASE(m_pSock);

    delete this;
}

inline int
ConnDispatch::Send(CHXServSocket* pSock, int newproc)
{
    if (newproc < 0)
    {
        newproc = proc->pc->streamer_info->BestStreamer(proc);
    }

    if (*g_pProcessStartInProgress)
    {
        newproc = -1;
    }

    pSock->ExitProc();
    if (newproc < 0)
    {
        if (waiters.IsEmpty())
        {
            CreateStreamerCallback* cb = new CreateStreamerCallback;
            proc->pc->dispatchq->send(proc, cb, PROC_RM_CONTROLLER);
        }
        waiters.AddHead((void*)pSock);

        return -1;
    }
    else
    {
        proc->pc->streamer_info->PlayerConnect(proc, newproc);

        cd_SockDispatchCallback* cb = new cd_SockDispatchCallback;
        pSock->AddRef();
        cb->m_pSock = pSock;

#if !defined WIN32
        cb->defunct = 0;

#ifdef SHARED_FD_SUPPORT
        if (!g_bSharedDescriptors)
#endif /* SHARED_FD_SUPPORT */
        {
            cb->fdp = ((CoreContainer*)proc->pc)->m_fdps[newproc];
            ASSERT(cb->fdp);

            cb->fd = pSock->GetSock();
            cb->fdp->Send(cb->fd, newproc);
        }
#endif

        proc->pc->dispatchq->send(proc, cb, newproc);
        return 0;
    }
}

inline void
ConnDispatch::process()
{
    CHXServSocket* pSock;
    int newproc;

    //XXXSMP What if streamers start so fast that waiters has more
    //  then the capacity of a streamer?
    while (!waiters.IsEmpty())
    {
        pSock = (CHXServSocket*)waiters.RemoveHead();
        newproc = proc->pc->streamer_info->BestStreamer(proc);
        if (Send(pSock, newproc) < 0)
            return;
    }
}

#endif
