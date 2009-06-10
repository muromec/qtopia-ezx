/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fdtrans_manager.cpp,v 1.7 2004/09/17 23:40:58 tmarshall Exp $
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

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxcomm.h"
#include "proc.h"
#include "dispatchq.h"

#include "server_engine.h"
#include "callback_container.h"
#include "base_callback.h"
#include "server_control.h"
#include "tcpio.h"
#include "inetwork.h"
#include "shmem.h"

#include "fdtrans_manager.h"

#include "core_container.h"
#include "conn_dispatch.h"

#include "rights_msg.h"

void
FDTransferManager::AddToFDBlock(Process* p, SockDispatchCallback* cb, int procnum)
{
    if (procnum > m_hiproc)
    {
        m_hiproc = procnum;
    }
    if (procnum < m_loproc)
    {
        m_loproc = procnum;
    }

    if (m_cbq[procnum].IsFull())
    {
        AttemptFDBlock(p, procnum);
    }
    m_cbq[procnum].Insert(cb);
}

void
FDTransferManager::AttemptFDBlock(Process* proc)
{
    int i;
    for (i = m_loproc; i <= m_hiproc; i++)
    {
        if (!m_cbq[i].IsEmpty())
        {
            AttemptFDBlock(proc, i);
        }
    }
}

void
FDTransferManager::AttemptFDBlock(Process* proc, int procnum)
{
    int fdcount;
    int newfdcount;

    rights_msg* prm;
    SockDispatchCallback* pcdc;
    SockDispatchCallback** pcdcq;

    newfdcount = fdcount = m_cbq[procnum].GetCount();
    prm = new rights_msg;
    prm->set_num_fd(fdcount);
    pcdcq = new SockDispatchCallback*[fdcount];

    int j = 0;
    while (!m_cbq[procnum].IsEmpty())
    {
        pcdc = m_cbq[procnum].Remove();
        if (pcdc->m_pSock)
        {
            // before passing the fd make sure that the socket is
            // alive, if not set the defunct flag so that when the
            // pcdc->Func() is called it knows to delete the
            // CloseFDCallback instead of dispatching it.
            if (pcdc->m_pSock->IsValid())
            {
                prm->add_fd(pcdc->fd);
            }
            else
            {
                newfdcount--;
                pcdc->defunct = 1;
            }
        }
        else
        {
            // for the proxy's lbl sockets
            prm->add_fd(pcdc->fd);
        }

        pcdcq[j++] = pcdc;
    }

    // in case there were dead clients in the list reset the # of
    // fds being sent across to the streamer
    if (fdcount != newfdcount)
        prm->reset_num_fd(newfdcount);

    if (pcdc->fdp->Send(prm->buf(), newfdcount + 1,
        pcdc->received_procnum) < 0)
    {
        int k = 0;
        for (j = 0; j < fdcount; j++)
        {
            pcdc = pcdcq[j];
            pcdc->defunct = 1;
            // schedule it for deletion
            // proc->pc->engine->schedule.enter(10000, pcdc);
            pcdc->Func();
            pcdc->Release();
        }
    }
    else
    {
        for (j = 0; j < fdcount && pcdcq[j]; j++)
        {
            pcdc = pcdcq[j];
            // if the fd was bad (defunct = 1) delete the
            // CloseFDCallback (rather than scheduling it)
            // via a call to pcdc->Func()
            if (pcdc->defunct)
            {
                pcdc->Func();
                pcdc->Release();
            }
            else // dispatch the callback to the appropriate streamer
            {
               /*
                * close the fd here so that there is no need to
                * dispatch a cb from the streamer back to the
                * core proc to close the fd later after the new
                * fd has been received in the streamer
                */
                close(pcdc->fd);
                proc->pc->dispatchq->send(proc, pcdc, procnum);
            }
        }
    }
    delete [] pcdcq;
    delete prm;
}

UINT32
FDTransferManager::GetCount(int procnum)
{
    return m_cbq[procnum].GetCount();
}
