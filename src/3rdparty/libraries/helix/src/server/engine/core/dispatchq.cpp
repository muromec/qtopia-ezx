/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: dispatchq.cpp,v 1.9 2007/05/24 08:36:37 ckarusala Exp $ 
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
/*
 *  dispatchq.cpp
 *
 *  moved implementation from dispatchq.h
 *
 */

#include "hxtypes.h"
#include "platform_config.h"
#include "hxcom.h"
#include "mutex.h"
#include "dispatchq.h"
#include "microsleep.h"
#include "hxassert.h"

#ifdef SHARED_FD_SUPPORT
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif /* _LINUX && SHARED_FD_SUPPORT */

#ifdef SHARED_FD_SUPPORT
extern int* g_pProcessPipes[];
#endif // SHARED_FD_SUPPORT

DispatchQueue::DispatchQueue()
{
#ifdef DISPATCHQ_RSS
    // m_ullSendTotal = 0;
    // m_ullRecvTotal = 0;
    // m_ullExecTotal = 0;
#endif
    memset(queues, 0, sizeof(SimpleCallback**) * MAX_THREADS);
    memset(top, 0, sizeof(int) * MAX_THREADS);
    memset(bottom, 0, sizeof(int) * MAX_THREADS);
    memset(size, 0, sizeof(int) * MAX_THREADS);
    memset((char*)m_pMutex, 0, sizeof(HX_MUTEX) * MAX_THREADS);
    memset(m_bHasDQ, FALSE, sizeof(BOOL) * MAX_THREADS);
#if defined _LINUX && defined SHARED_FD_SUPPORT
    memset(m_bSendTrigger, 0, sizeof(UINT32) * MAX_THREADS);
#else
    memset(m_bExecuted, TRUE, sizeof(BOOL) * MAX_THREADS);
#endif /* _LINUX && SHARED_FD_SUPPORT */

#ifdef DISPATCHQ_RSS
    memset(m_pTriggersSent, 0, sizeof(UINT32) * MAX_THREADS);
    memset(m_pTriggersRcvd, 0, sizeof(UINT32) * MAX_THREADS);
    memset(m_pSendCounter, 0, sizeof(UINT32) * MAX_THREADS);
    memset(m_pRecvCounter, 0, sizeof(UINT32) * MAX_THREADS);
    memset(m_pExecCounter, 0, sizeof(UINT32) * MAX_THREADS);
#endif
}

DispatchQueue::~DispatchQueue()
{
    HX_ASSERT(0 && "DispatchQueue should not be deleted!");
}

void
DispatchQueue::init(Process* proc)
{
    int procnum = proc->procnum();
    queues[procnum] = new SimpleCallback*[QUEUE_SIZE];
    memset(queues[procnum], 0, sizeof(SimpleCallback*) * QUEUE_SIZE);
    m_pMutex[procnum] = HXCreateMutex();
    m_bHasDQ[procnum] = TRUE;
#if defined _LINUX && defined SHARED_FD_SUPPORT
    m_bSendTrigger[procnum] = 1;
#endif /* _LINUX && SHARED_FD_SUPPORT */

#if defined SHARED_FD_SUPPORT
    int flags = 0;
    int writefd = g_pProcessPipes[procnum][1];
    fcntl(writefd, F_GETFL, &flags);
    flags |= O_NONBLOCK;
    if (fcntl(writefd, F_SETFL, flags) < 0)
    {
	fprintf(stderr, "fcntl(pipe[%d:%lu][fd(%d)], O_NONBLOCK) failed: %s", procnum,
	    proc->procid(procnum), writefd, strerror(errno));
    }
#endif /* SHARED_FD_SUPPORT */
}

int
DispatchQueue::send(Process* from_proc, SimpleCallback* m, int to_procnum)
{
    int bNeedToKill = 0;
    int from_procnum = from_proc->procnum();
    PROCID_TYPE to_tid = from_proc->procid(to_procnum);
    PROCID_TYPE from_tid = from_proc->procid(from_procnum);

    DPRINTF(D_XFER, ("Sending DispatchQueue Message from %d to %d\n", 
            from_tid, to_tid));
#ifdef DEBUG
    if (from_proc->numprocs() < to_procnum)
	PANIC(("DispatchQueue::send() - Sending to a nonexistent process\n"));
#endif

    HXMutexLock(m_pMutex[to_procnum]);
    
    if (size[to_procnum] == QUEUE_SIZE)
    {
        fprintf(stderr, "DispatchQueue::send: size == QUEUE_SIZE "
                "(to_procnum=%d, bottom=%d, top=%d) on message from %d to %d\n",
                to_procnum, bottom[to_procnum], top[to_procnum],
                from_tid, to_tid);
	HXMutexUnlock(m_pMutex[to_procnum]);
	return -1;
    }

    DPRINTF(D_XFER, ("from %d to %d -- queues[%d][%d] = %p\n",
            from_tid, to_tid, to_procnum, top[to_procnum], m));

    queues[to_procnum][top[to_procnum]] = m;

    // if the target has started executing callbacks since the last send
    // we need to kill it to tell it more have been added
#if defined _LINUX && defined SHARED_FD_SUPPORT
    if (m_bSendTrigger[to_procnum])
    {
	bNeedToKill = 1;
        HXAtomicDecUINT32(&m_bSendTrigger[to_procnum]);
#ifdef DISPATCHQ_RSS
        m_pTriggersSent[to_procnum]++;
#endif
    }
#else /* !_LINUX || !SHARED_FD_SUPPORT */
    if (m_bExecuted[to_procnum])
    {
      bNeedToKill = 1;
      m_bExecuted[to_procnum] = FALSE;
#ifdef DISPATCHQ_RSS
        m_pTriggersSent[to_procnum]++;
#endif
    }
#endif /* _LINUX && SHARED_FD_SUPPORT */

    if (top[to_procnum] == QUEUE_SIZE - 1)
        top[to_procnum] = 0;
    else
        top[to_procnum]++;

    size[to_procnum]++;
#ifdef DISPATCHQ_RSS
    m_pSendCounter[from_procnum]++;
    m_pRecvCounter[to_procnum]++;
#endif

    HXMutexUnlock(m_pMutex[to_procnum]);

#ifdef DISPATCHQ_RSS
    // HXAtomicIncUINT32(m_pSendCounter+MAX_THREADS-1);
    // HXAtomicIncUINT32(m_pRecvCounter+MAX_THREADS-1);
    // m_ullSendTotal++;
    // m_ullRecvTotal++;
#endif
    if (bNeedToKill)
    {
#ifdef SHARED_FD_SUPPORT
        if (g_bSharedDescriptors)
        {
            char x = 0;
	    int writefd = g_pProcessPipes[to_procnum][1];
	    int n = write(writefd, &x, 1);
	    if (n < 0 && errno != EAGAIN)
	    {
		fprintf(stderr, "E: write(pipe[%d:%lu][fd(%d)]) from proc(%d:%lu) - failed(%ld) - %s\n",
		    to_procnum, to_tid, writefd, from_procnum, from_tid, errno, strerror(errno));
	    }
        }
        else
#endif /* SHARED_FD_SUPPORT */
        {
	    DPRINTF(D_XFER, ("Sending SIGUSR1 from %d to %d\n", 
                from_tid, to_tid));
            kill(to_tid, SIGUSR1);
        }
    }
    DPRINTF(D_XFER, ("from %d to %d -- top[%d] = %d\n",
            from_tid, to_tid, to_procnum, top[to_procnum]));
    return 0;
}

int
DispatchQueue::execute(Process* proc)
{
    int queueNum = proc->procnum();

    HXMutexLock(m_pMutex[queueNum]);

    // grab start and end positions and number of callbacks to execute
    int index = bottom[queueNum];
    int end = top[queueNum];
    int numberToExecute = size[queueNum];

#if !defined _LINUX || !defined SHARED_FD_SUPPORT
    // indicate that we've begun processing so that send will know
    // that it needs to kill again when it adds new callbacks
    m_bExecuted[queueNum] = TRUE;
#endif

    HXMutexUnlock(m_pMutex[queueNum]);

    while (numberToExecute)
    {
        HXMutexLock(m_pMutex[queueNum]);
        // remove the callback from the queue
        SimpleCallback* cb = queues[queueNum][index];
        queues[queueNum][index] = 0;

        if (index == QUEUE_SIZE - 1)
            index = 0;
        else
            index++;

        // update the bottom and the size which will potentially allow more
        // callbacks to be put on the queue if it's nearly full
        bottom[queueNum] = index;
        size[queueNum]--;
#ifdef DISPATCHQ_RSS
	m_pExecCounter[queueNum]++;
#endif

        HXMutexUnlock(m_pMutex[queueNum]);

#ifdef DISPATCHQ_RSS
      // HXAtomicIncUINT32(m_pExecCounter+MAX_THREADS-1);
      // m_ullExecTotal++;
#endif
        cb->func(proc);
        
        numberToExecute--;
    }

    return 1;
}

int
DispatchQueue::cancel_crashed_func(Process* proc)
{
    //
    // This function doesn't need to do anything anymore
    //
    return 1;
}

#ifdef DISPATCHQ_RSS
void
DispatchQueue::printCounters(Process* proc, char*& pBuf)
{
    int i;

    pBuf += sprintf(pBuf, "    DispatchQSend:  ");
    for (i = 0; i < proc->numprocs(); i++)
    {
	pBuf += sprintf(pBuf, "%6ld ", m_pSendCounter[i]);
    }
    // pBuf += sprintf(pBuf, "%6ld ", m_pSendCounter[MAX_THREADS-1]);
    // pBuf += sprintf(pBuf, "%6ld ", m_ullSendTotal);
    pBuf += sprintf(pBuf, "\n");

    pBuf += sprintf(pBuf, "    DispatchQRecv:  ");
    for (i = 0; i < proc->numprocs(); i++)
    {
	pBuf += sprintf(pBuf, "%6ld ", m_pRecvCounter[i]);
    }
    // pBuf += sprintf(pBuf, "%6ld ", m_pRecvCounter[MAX_THREADS-1]);
    // pBuf += sprintf(pBuf, "%6ld ", m_ullRecvTotal);
    pBuf += sprintf(pBuf, "\n");

    pBuf += sprintf(pBuf, "    DispatchQExec:  ");
    for (i = 0; i < proc->numprocs(); i++)
    {
	pBuf += sprintf(pBuf, "%6ld ", m_pExecCounter[i]);
    }
    // pBuf += sprintf(pBuf, "%6ld ", m_pExecCounter[MAX_THREADS-1]);
    // pBuf += sprintf(pBuf, "%6ld ", m_ullExecTotal);
    pBuf += sprintf(pBuf, "\n");

    pBuf += sprintf(pBuf, "    DQTriggersSent: ");
    for (i = 0; i < proc->numprocs(); i++)
    {
	pBuf += sprintf(pBuf, "%6ld ", m_pTriggersSent[i]);
    }
    pBuf += sprintf(pBuf, "\n");

    pBuf += sprintf(pBuf, "    DQTriggersRcvd: ");
    for (i = 0; i < proc->numprocs(); i++)
    {
	pBuf += sprintf(pBuf, "%6ld ", m_pTriggersRcvd[i]);
    }
    pBuf += sprintf(pBuf, "\n");

    pBuf += sprintf(pBuf, "    DispatchQSize:  ");
    for (i = 0; i < proc->numprocs(); i++)
    {
	pBuf += sprintf(pBuf, "%6ld ", size[i]);
    }
    pBuf += sprintf(pBuf, "\n");

    memset(m_pSendCounter, 0, sizeof(UINT32) * MAX_THREADS);
    memset(m_pRecvCounter, 0, sizeof(UINT32) * MAX_THREADS);
    memset(m_pExecCounter, 0, sizeof(UINT32) * MAX_THREADS);
    memset(m_pTriggersSent, 0, sizeof(UINT32) * MAX_THREADS);
    memset(m_pTriggersRcvd, 0, sizeof(UINT32) * MAX_THREADS);
}
#endif
