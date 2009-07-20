/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: lbl_cdispatch.h,v 1.10 2007/03/05 23:24:06 atin Exp $
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

#ifndef _LBL_CDISPATCH_H_
#define _LBL_CDISPATCH_H_

#include "debug.h"
#include "platform_config.h"
#include "streamer_info.h"
#include "_main.h"
#include "simple_callback.h"
#include "base_callback.h"
#include "dispatchq.h"
#include "hxslist.h"
#include "fdpass_socket.h"
#include "core_container.h"
#include "sockfd.h"
#include "sockio.h"
#include "tcpio.h"
#include "inetwork.h"
#include "hxslist.h"
#include "misc_container.h"

#include "common_dispatch.h"

#if defined _LINUX || defined PTHREADS_SUPPORTED
#    define SHARED_FD_SUPPORT
#endif // _LINUX || PTHREADS_SUPPORTED

#if defined SHARED_FD_SUPPORT
extern BOOL g_bSharedDescriptors;
#endif // SHARED_FD_SUPPORT

class LBLAcceptor;

class lbl_ClientDispatchCallback : public SockDispatchCallback
{
public:
    IHXTCPSocketContext*        tcpsc;
    IHXListenResponse*          resp;
    void func(Process* proc);
    STDMETHODIMP Func(THIS)
    {
#ifndef _WIN32
        if (defunct)
            if (tcpsc)
            {
                tcpsc->Release();
                tcpsc = 0;
            }
#endif /* _WIN32 */
        return HXR_OK;
    }
};

class LBLConnDispatch {
public:
                        LBLConnDispatch(Process* _proc, LBLAcceptor* pAcceptor);
    int                 Send(IHXTCPSocketContext* c);
    void                process();
    volatile BOOL                m_bStartInProgress;
private:

    Process*            proc;
    LBLAcceptor*        m_pAcceptor;
    CHXSimpleList       waiters;
};

inline
LBLConnDispatch::LBLConnDispatch(Process* _proc, LBLAcceptor* pAcceptor)
{
    proc = _proc;
    m_pAcceptor = pAcceptor;
    m_bStartInProgress = FALSE;
}

inline void
lbl_ClientDispatchCallback::func(Process* proc)
{
    DPRINTF(D_INFO, ("Go Go Gadget Reconnect\n"));

#if !defined WIN32 && !defined SHARED_FD_SUPPORT

    int fd = fdp->Recv(proc->procnum());

    if (fd < 0)
    {
        goto CloseOtherSide;
    }
    ((SocketIO *)tcpsc->getReadTCPIO())->fd(fd);
#endif // !defined WIN32 && !defined SHARED_FD_SUPPORT

#ifdef SHARED_FD_SUPPORT
    if (!g_bSharedDescriptors)
    {
        int fd = fdp->Recv(proc->procnum());

        if (fd < 0)
        {
            goto CloseOtherSide;
        }
        ((SocketIO *)tcpsc->getReadTCPIO())->fd(fd);
    }
#endif /* SHARED_FD_SUPPORT */

    tcpsc->reconnect(proc->pc->engine);
    //tcpsc->enableRead();
    resp->NewConnection(HXR_OK, tcpsc);
    tcpsc->Release();
    tcpsc = 0;

#if !defined WIN32
CloseOtherSide:
#endif

    delete this;
}

inline int
LBLConnDispatch::Send(IHXTCPSocketContext* c)
{
    IHXListenResponse* pResponse = 0;
    PluginHandler::Plugin* pPlugin = 0;
    int best;

    if (m_bStartInProgress)
    {
        best = -1;
    }
    else
    {
        best = m_pAcceptor->GetBestProcess(pResponse, pPlugin);
    }

    if (best < 0)
    {
        if (waiters.IsEmpty())
        {
            CreateMiscProcCallback* cb = new CreateMiscProcCallback();
            cb->m_plugin = pPlugin;
            cb->m_bNeedSocket = pPlugin->m_generic &&
                (pPlugin->m_load_multiple);
            cb->m_pLBLAcceptor = m_pAcceptor;
            m_bStartInProgress = TRUE;
            proc->pc->dispatchq->send(proc, cb, PROC_RM_CONTROLLER);
        }
        c->disableRead();
        waiters.AddHead((void*)c);

        return -1;
    }
    else
    {
        DPRINTF(0x01000000, ("Dispatching Connection from proc #%d to #%d!\n",
                proc->procnum(), best));

        c->disconnect();
        lbl_ClientDispatchCallback* cb = new lbl_ClientDispatchCallback;
        cb->m_pSock = 0; // is useful in the obsolete fdtrans_manager on solaris
        cb->tcpsc = c;
        cb->resp = pResponse; /* DON'T ADDREF!! You're in the wrong Process */
#if !defined WIN32
#  ifdef SHARED_FD_SUPPORT
        if (!g_bSharedDescriptors)
#  endif /* SHARED_FD_SUPPORT */
        {
            cb->fdp    = ((CoreContainer*)proc->pc)->m_fdps[best];
            ASSERT(cb->fdp);

            cb->fd = ((SocketIO *)c->getReadTCPIO())->fd();

            if (cb->fdp->Send(((SocketIO *)c->getReadTCPIO())->fd(), best) < 0)
            {
                DPRINTF(D_ERROR, ("LBLConnDispatch::Send: errno %d returned by sendmsg\n", errno));
                c->Release();
                delete cb;
                return -1;
            }
        }
#endif // !defined WIN32

        proc->pc->dispatchq->send(proc, cb, best);

        return 0;
    }
}

inline void
LBLConnDispatch::process()
{
    IHXTCPSocketContext* c;

    //XXXSMP What if connections happen so fast that waiters has more
    //  then the capacity of a misc process?
    while (!waiters.IsEmpty())
    {
        c = (IHXTCPSocketContext*)waiters.RemoveHead();

        if (Send(c) < 0)
            return;
    }
}

#endif // _LBL_CDISPATCH_H_
