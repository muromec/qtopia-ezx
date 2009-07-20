/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: server_engine.cpp,v 1.26 2008/07/03 21:54:17 dcollins Exp $
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

#include "hxtypes.h"
#include "platform_config.h"
#include <stdio.h>
#include <errno.h>
#ifdef _UNIX
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#else
#include "hlxclib/windows.h"
#endif
#ifdef _SOLARIS
#include <alloca.h>
#endif
#include "hlxclib/signal.h"

#include "hxcom.h"
#include "hxerror.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "proc.h"
#include "dispatchq.h"
#include "mutex.h"
#include "timeval.h"
#include "servpq.h"
#include "chxpckts.h"
#include "microsleep.h"
#include "loadinfo.h"

#include "base_errmsg.h"
#include "base_callback.h"
#include "callback_container.h"
#include "callback.h"
#include "server_control.h"
#include "shmem.h"
#include "server_engine.h"
#include "servreg.h"
#include "mutexlist.h"
#include "logoutputs.h"
#include "hxrssmgr.h"
#include "rssmgr.h"
#ifdef _UNIX
#include "hup_response.h"
#include <sys/types.h>
#include <sys/mman.h>
#endif

#if defined _HPUX || defined _OSF1
#include <alloca.h>
#elif defined _LSB
#define alloca(size)   __builtin_alloca (size)
#endif

#include "server_context.h"
#include "globals.h"
#include "safestring.h"

#ifdef PAULM_SOCKTIMING
#include "sockettimer.h"
SocketTimer g_SocketTimer;
#endif

#ifdef PAULM_REPORT
extern int g_DoSocketTimes;
#endif

#ifdef PAULM_CLIENTTIMING
#include "classtimer.h"
extern ClassTimer g_ClientTimer;
#endif

#ifdef PAULM_SESSIONTIMING
#include "classtimer.h"
extern ClassTimer g_SessionTimer;
#endif

#ifdef PAULM_CHPTIMING
#include "classtimer.h"
extern ClassTimer g_CHPTimer;
#endif

#ifdef PAULM_RTSPPROTTIMING
#include "classtimer.h"
extern ClassTimer g_RTSPProtTimer;
#endif

#ifdef PAULM_NCPTIMING
#include "classtimer.h"
extern ClassTimer g_NCPTimer;
#endif

#ifdef PAULM_CSPTIMING
#include "classtimer.h"
extern ClassTimer g_CSPTimer;
#endif

#ifdef PAULM_TCPSCTIMING
#include "classtimer.h"
extern ClassTimer g_TCPSCTimer;
#endif

#ifdef PAULM_CSTCPSCTIMING
#include "classtimer.h"
extern ClassTimer g_CSTCPSCTimer;
#endif

#ifdef PAULM_STORERECTIMING
#include "classtimer.h"
extern ClassTimer g_STORERECTimer;
#endif

#ifdef PAULM_RTSPPTIMING
#include "classtimer.h"
extern ClassTimer g_RTSPPTimer;
#endif

#ifdef PAULM_RTSPSTTIMING
#include "classtimer.h"
extern ClassTimer g_RTSPSTTimer;
#endif

#ifdef PAULM_INTCPCTIMING
#include "classtimer.h"
extern ClassTimer g_INTCPCTimer;
#endif

#ifdef PAULM_PLAYERTIMING
#include "classtimer.h"
extern ClassTimer g_PlayerTimer;
#endif

#ifdef PAULM_STREAMDATATIMING
#include "classtimer.h"
extern ClassTimer g_StreamDataTimer;
#endif

#ifdef PAULM_PPMTIMING
#include "classtimer.h"
extern ClassTimer g_PPMTimer;
#endif

#ifdef PAULM_TNGTCPTRANSTIMING
#include "classtimer.h"
extern ClassTimer g_TNGTCPTransTimer;
#endif

#ifdef PAULM_HXBUFFERSPACE
extern UINT32 GetBufferDataSize();
#endif

#ifdef PAULM_HXSTRINGSPACE
extern UINT32 GetStringDataSize();
#endif

extern Engine** volatile g_ppLastEngine;
extern BOOL terminated;
extern BOOL g_bServerGoingDown;

ServerEngine* get_my_engine();

#ifdef SHARED_FD_SUPPORT
int* g_pProcessPipes[MAX_THREADS];
#endif /* SHARED_FD_SUPPORT */

#ifdef PAULM_TCPSC_LOSTADDREF
#define NUM_LOST_ADDREFS 50
char* g_lostAddrefs[NUM_LOST_ADDREFS];
UINT32 g_lostAddrefCount = 0;

void
handleLostAddref()
{
    if (g_lostAddrefCount == NUM_LOST_ADDREFS)
    {
        return;
    }
    char* buh = get_trace();
    UINT32 i;
    for (i = 0; i < g_lostAddrefCount; i++)
    if (!strcmp(g_lostAddrefs[i], buh))
    {
        return;
    }
    g_lostAddrefs[g_lostAddrefCount] = new char[strlen(buh) + 1];
    strcpy(g_lostAddrefs[g_lostAddrefCount], buh);
    g_lostAddrefCount++;
}

void
dumpLostAddrefs()
{
    printf("Any lost IHXTCPSocketContext addrefs coming next...\n");
    if (g_lostAddrefCount == NUM_LOST_ADDREFS)
    {
        printf("LostAddref OVERFLOW!\n");
    }
    UINT32 i;
    for (i = 0; i < g_lostAddrefCount; i++)
    {
        printf("lostAddref %d:\n", i);
        printf("%s\n", g_lostAddrefs[i]);
        fflush(stdout);
    }
}

#endif

#ifdef _UNIX

static int trigger[2];

void
foo()
{
    char x = 0;
    write(trigger[1], &x, 1);
}

#endif

ServerEngine::ServerEngine(Process* _proc)
    : proc(_proc)
{
    proc->set_engine(this);
#ifdef _UNIX

    int temp[2];
    pipe(temp);

#if defined(HELIX_FEATURE_SERVER) && defined(_SOLARIS)
     if (temp[0] < 256)
     {
         int nNewFD = fcntl(temp[0], F_DUPFD, 256);
         int nTemp = temp[0];
         temp[0] = nNewFD;
         close(nTemp);
     }

     if (temp[1] < 256)
     {
         int nNewFD = fcntl(temp[1], F_DUPFD, 256);
         int nTemp = temp[1];
         temp[1] = nNewFD;
         close(nTemp);
     }

#endif //defined(HELIX_FEATURE_SERVER) && defined (_SOLARIS)

#ifdef SHARED_FD_SUPPORT
    if (g_bSharedDescriptors)
    {
        g_pProcessPipes[proc->procnum()] = new int[2];
        g_pProcessPipes[proc->procnum()][0] = temp[0];
        g_pProcessPipes[proc->procnum()][1] = temp[1];
      DQTriggerCallback* cb = new DQTriggerCallback(proc, temp[0]);
        callbacks.add(HX_READERS, temp[0], cb, TRUE);
    }
    else
#endif /* SHARED_FD_SUPPORT */
    {
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
	sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))foo;
	sigaction(SIGUSR1, &sa, NULL);

        HX_VERIFY(dup2(temp[0], 31) == 31);
        HX_VERIFY(dup2(temp[1], 32) == 32);
        trigger[0] = 31;
        trigger[1] = 32;

        close(temp[0]);
        close(temp[1]);
      DQTriggerCallback* cb = new DQTriggerCallback(proc, trigger[0]);
        callbacks.add(HX_READERS, trigger[0], cb, TRUE);
    }

#ifdef HX_SELECT
    m_poll_info.pfds = 0;
    m_poll_info.size = 0;
#endif
#endif

    m_bMutexProtection = FALSE;
    m_CrashState = NONE;
}

void
ServerEngine::preevent()
{
}

void
ServerEngine::postevent()
{
}

void
ServerEngine::Dispatch()
{
    proc->pc->dispatchq->execute(proc);
}

#ifdef HX_SELECT

int ServerEngine::grow_poll(int nfds)
{
    if (nfds >= HX_MAX_POLL_SIZE)
        return -1;

    if (m_poll_info.pfds)
    {
        HX_VECTOR_DELETE(m_poll_info.pfds);
    }

    if (!m_poll_info.size)
        m_poll_info.size = 1024;

    while (m_poll_info.size < nfds)
        m_poll_info.size = m_poll_info.size << 1;

    if (m_poll_info.size > HX_MAX_POLL_SIZE)
        return -1;

    m_poll_info.pfds = new struct pollfd [m_poll_info.size];
    if (!m_poll_info.pfds)
        return -1;

    //printf("ServerEngine::grow_poll pid %d, new size %d\n", getpid(), m_poll_info.size);
    return m_poll_info.size;
}

int
ServerEngine::select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* errorfds,
           struct timeval* timeout)
{
    int fd = 0;
    struct pollfd* pf = 0;
    nfds_t npoll = 0;
    int msec = -1;              /* infinite time */
    int nbits = -1;
    int nmask;
    int i;
    int fdbase;

    if (nfds < 0 || nfds >= HX_MAX_POLL_SIZE ||
        (nfds > m_poll_info.size && grow_poll(nfds) == -1))
    {
        errno = EINVAL;
        return -1;
    }
    nmask = howmany(nfds, FD_NFDBITS);
    pf = m_poll_info.pfds;
    for (i = 0, fdbase = 0; i < nmask; ++i, fdbase += FD_NFDBITS)
    {
        fds_mask rd_mask = 0;
        fds_mask wr_mask = 0;
        fds_mask er_mask = 0;
        int k = 0;

        if (readfds)
        {
            rd_mask = readfds->fds_bits[i];
            readfds->fds_bits[i] = 0;
        }
        if (writefds)
        {
            wr_mask = writefds->fds_bits[i];
            writefds->fds_bits[i] = 0;
        }
        if (errorfds)
        {
            er_mask = errorfds->fds_bits[i];
            errorfds->fds_bits[i] = 0;
        }
        if (rd_mask || wr_mask || er_mask) {
            for (; k < FD_NFDBITS; ++k) {
                short events = 0;
                int bit = 1<<k;
                if (rd_mask&bit) events |= HX_EVENT_READ;
                if (wr_mask&bit) events |= HX_EVENT_WRITE;
                if (er_mask&bit) events |= HX_EVENT_ERROR;

                if (events) {
                    pf->fd = fdbase + k;
                    pf->events = events;
                    pf->revents = 0;
                    ++pf;
                }
            }
        }
    }
    npoll = pf - m_poll_info.pfds;
    if (timeout)
    {
        if (timeout->tv_sec > HX_TIMEVAL_MAXSEC ||
            timeout->tv_usec > HX_TIMEVAL_MAXUSEC)
        {
            errno = EINVAL;
            return -1;
        }
        msec = timeout->tv_sec*1000 + timeout->tv_usec/1000;
    }
    if (poll(m_poll_info.pfds, npoll, msec) != -1)
    {
        struct pollfd* pfds_end = m_poll_info.pfds + npoll;
        int fdnext = FD_NFDBITS;
        nbits = 0;
        for (pf = m_poll_info.pfds, i = 0, fdbase = 0; pf < pfds_end; ++pf)
        {
            int fd = pf->fd;
            while (fdnext <= fd)
            {
                fdbase = fdnext;
                fdnext += FD_NFDBITS;
                ++i;
            }
            if (readfds && (pf->events&HX_EVENT_READ) &&
                (pf->revents&HX_REVENT_READ))
            {
                readfds->fds_bits[i] |= 1<<(fd-fdbase);
                ++nbits;
            }
            if (writefds && (pf->events&HX_EVENT_WRITE) &&
                (pf->revents&HX_REVENT_WRITE))
            {
                writefds->fds_bits[i] |= 1<<(fd-fdbase);
                ++nbits;
            }
            if (errorfds && (pf->events&HX_EVENT_ERROR) &&
                (pf->revents&HX_REVENT_ERROR))
            {
                errorfds->fds_bits[i] |= 1<<(fd-fdbase);
                ++nbits;
            }
        }
    }
    return nbits;
}
#endif

void
ServerEngine::mainloop(BOOL bJumpStart)
{
#if defined _LINUX || defined _SOLARIS
    if (sigsetjmp(m_JumpStartJumpBuffer, 1))
    {
        bJumpStart = TRUE;
    }
#endif // defined _LINUX || defined _SOLARIS

    volatile unsigned int guard1 = 0xc001d00d;
    int n;
    Timeval*    timeoutp;
    Timeval     timeout;
    Timeval     timeout2;
    Timeval     last_left_select_time;
    Timeval     last_in_select_time;
    volatile unsigned int guard2 = 0xcc110088;
    UINT32 ulErrCode = 0;
    UINT32 ulErrCounter = 0;
    int procnum = proc->procnum();

#ifndef _WIN32
    /*
     * For security reasons, we make the stack a bit unpredictable.
     * While this affords us little protection, it's better then nothing.
     * Without this code, our stack frames are extremely predictable when
     * parsing network data.
     *
     * XXXSMP This should work on WIN32 too, but PNCRT lacks it?
     */
    srand((unsigned)time(NULL));
    volatile void* sp = alloca(rand() % (8192) + 8192 + now.tv_usec % 1024);
#endif
#if 0
    /*
     * Do not allow code on the stack
     * XXXSMP Hope that the stack grows downward!!
     */
    mprotect(sp - 1024 * 4000, 1024 * 4000, PROT_READ | PROT_WRITE);
#endif

    proc->pc->server_context->QueryInterface(IID_IHXFastAlloc,
        (void **)&schedule.m_pFastAlloc);
    proc->pc->server_context->QueryInterface(IID_IHXFastAlloc,
        (void **)&ischedule.m_pFastAlloc);

    if (bJumpStart)
    {
        m_bForceSelect = TRUE;
        if (m_bMutexProtection)
        {
            m_bMutexProtection = FALSE;
            HXMutexUnlock(g_pServerMainLock);
        }

        bJumpStart = FALSE;

#ifdef _UNIX
        if (sigsetjmp(m_JumpBuffer, 1))
        {
            goto RealCrashAvoid;
        }
#else
        if (setjmp(m_JumpBuffer))
        {
            goto RealCrashAvoid;
        }
#endif // _UNIX

        /* Jump back Into the engine */
        goto restart_after_GPF;
    }

#ifdef _UNIX
    if (sigsetjmp(m_JumpBuffer, 1))
#else
    if (setjmp(m_JumpBuffer))
#endif // _UNIX
    {
RealCrashAvoid:
        char buffer[4096];
        if (guard1 != 0xc001d00d || guard2 != 0xcc110088)
        {
            sprintf(buffer, "Jump Start Engine: Damaged Stack!\n");
            if (g_bCrashAvoidancePrint)
            {
                printf ("%s", buffer);
                fflush(0);
            }
            RSSManager::CrashOutput(buffer);
            ServerEngine* pEngine = get_my_engine();
            pEngine->mainloop(TRUE);
        }
        if (proc->pc->registry->m_nLockedBy == procnum)
        {
            sprintf(buffer, "Server Registry: Recovering!\n");
            if (g_bCrashAvoidancePrint)
            {
                printf ("%s", buffer);
                fflush(0);
            }
            RSSManager::CrashOutput(buffer);
            proc->pc->registry->m_nLockedBy = 0;
            HXMutexUnlock(proc->pc->registry->m_pMutex);
        }
        if (proc->pc->managed_mutex_list && proc->pc->managed_mutex_list->LockCount() > 0)
        {
            sprintf(buffer, "Managed Locks: Recovering %lu locks!\n",
                        proc->pc->managed_mutex_list->LockCount());
            if (g_bCrashAvoidancePrint)
            {
                printf ("%s", buffer);
                fflush(0);
            }
            RSSManager::CrashOutput(buffer);
            proc->pc->managed_mutex_list->UnlockAll();
        }
        if (m_CrashState == NONE)
        {
            sprintf(buffer, "Jump Start Engine: Unknown Crash State!\n");
            if (g_bCrashAvoidancePrint)
            {
                printf ("%s", buffer);
                fflush(0);
            }
            RSSManager::CrashOutput(buffer);
            ServerEngine* pEngine = get_my_engine();
            pEngine->mainloop(TRUE);
        }
        else if (m_CrashState == IN_LOOP)
        {
            sprintf(buffer, "Normal CA: In Scheduler/NetEvents!\n");
            if (g_bCrashAvoidancePrint)
            {
                printf ("%s", buffer);
                fflush(0);
            }
            RSSManager::CrashOutput(buffer);
            m_bForceSelect = TRUE;
            if (m_bMutexProtection)
            {
                m_bMutexProtection = FALSE;
                HXMutexUnlock(g_pServerMainLock);
            }

            /* Jump back Into the engine */
            goto restart_after_GPF;
        }
        else if (m_CrashState == IN_DISPATCHQ)
        {
            sprintf(buffer, "Normal CA: In DispatchQ\n");
            if (g_bCrashAvoidancePrint)
            {
                printf ("%s", buffer);
                fflush(0);
            }
            RSSManager::CrashOutput(buffer);
            m_bForceSelect = TRUE;
            if (proc->pc->process_type == PTWorker)
            {
                HXMutexLock(g_pServerMainLock);
                m_bMutexProtection = TRUE;
            }

            ASSERT (m_bMutexProtection);

            proc->pc->dispatchq->cancel_crashed_func(proc);

            /* Jump back Into the engine */
            goto restart_dispatchq_after_GPF;
        }
        else if (m_CrashState == IN_SERVERPQ)
        {
            sprintf(buffer, "Normal CA: In Non-Thread-safe ServerPQ\n");
            if (g_bCrashAvoidancePrint)
            {
                printf ("%s", buffer);
                fflush(0);
            }
            RSSManager::CrashOutput(buffer);
            m_bForceSelect = TRUE;

            schedule.crash_recovery();

            /* Jump back Into the engine */
            goto restart_serverpq_after_GPF;
        }
        else if (m_CrashState == IN_ISERVERPQ)
        {
            sprintf(buffer, "Normal CA: In Thread-safe ServerPQ\n");
            if (g_bCrashAvoidancePrint)
            {
                printf ("%s", buffer);
                fflush(0);
            }
            RSSManager::CrashOutput(buffer);
            m_bForceSelect = TRUE;

            ischedule.crash_recovery();

            /* Jump back Into the engine */
            goto restart_serverpq_after_GPF;
        }
    }

    *g_ppLastEngine = this;

    for(;;)
    {
#ifdef _WIN32
        if (terminated)
        {
            preevent();
            ExitThread(0);
        }
#endif

        if (schedule.empty() && ischedule.empty())
        {
            timeout = Timeval(600000, 0);
            timeoutp = &timeout;
        }
        else
        {
            timeoutp = &timeout;
            GETTIMEOFDAY(now);
            timeout = schedule.empty() ? Timeval(600000, 0) : schedule.head_time() - now;
            timeout2 = ischedule.empty() ? Timeval(600000, 0) : ischedule.head_time() - now;
            if (timeout2 < timeout)
                timeout = timeout2;
            DPRINTF(D_TIMEOUT, ("timeout: %ld.%06ld, (%ld.%06ld) from now\n",
                    schedule.head_time().tv_sec%100,
                    schedule.head_time().tv_usec,
                    timeout.tv_sec%100, timeout.tv_usec));
        }


        if (timeout < 0.0)
        {
            timeout = Timeval(0.0);
        }

        last_in_select_time = now;

        n = callbacks.Select((struct timeval*)timeoutp);
        if (n < 0)
        {
#ifdef _WIN32
            ulErrCode = WSAGetLastError();
#else
            ulErrCode = errno;
#endif
        }
        m_ulMainloopIterations++;

        GETTIMEOFDAY(now);
        last_left_select_time = now;

        DPRINTF(D_SELECT, ("select: %d (%ld.%06ld)\n", n,
                now.tv_sec%100, now.tv_usec));

        if (proc->pc->process_type == PTStreamer)
        {
            proc->pc->loadinfo->StreamerIteration();
        }

restart_serverpq_after_GPF:
        m_CrashState = IN_ISERVERPQ;
        m_pICurrentElem = ischedule.get_execute_list(now);
        m_CrashState = IN_SERVERPQ;
        m_pSCurrentElem = schedule.get_execute_list(now);
        m_CrashState = NONE;
        if (n < 0)
        {
            ulErrCounter++;
            if (ulErrCounter < 1000 && !g_bServerGoingDown)
            {
                char buf[256] = "\0";
                SafeSprintf(buf, 256, "select error in mainloop = %u , timeout: %ld.%06ld, procnum = %d\n",
                        ulErrCode, timeout.tv_sec, timeout.tv_usec, procnum);
                proc->pc->error_handler->Report(HXLOG_ERR, HXR_FAIL, (ULONG32)HXR_FAIL, buf, 0);
            }

            m_bMoreReaderOrWriter = FALSE;
            m_bMoreTSReaderOrWriter = FALSE;

            DPRINTF(D_SELECT, ("select: %s, timeout: %ld.%06ld\n",
                    strerror(errno), timeout.tv_sec, timeout.tv_usec));
#ifdef _UNIX
            if (errno == EBADF)
            {
                callbacks.HandleBadFds(proc->pc->error_handler);
            }
#endif
#ifdef _WIN32
            if (ulErrCode == WSAENOTSOCK)
            {
                callbacks.HandleBadFds(proc->pc->error_handler);
            }
#endif            
        }
        else if (n == 0)
        {
            m_bMoreReaderOrWriter = FALSE;
            m_bMoreTSReaderOrWriter = FALSE;
        }
        else
        {
            m_bMoreReaderOrWriter = TRUE;
            m_bMoreTSReaderOrWriter = TRUE;
            callbacks.invoke_start();
        }

        m_bOnce = proc->pc->dispatchq->count(procnum) > 0 ? TRUE : FALSE;
        m_bForceSelect = FALSE;

restart_after_GPF:
        m_CrashState = IN_LOOP;
        while (m_pICurrentElem || m_pSCurrentElem || m_bMoreReaderOrWriter ||
               m_bMoreTSReaderOrWriter || m_bOnce)
        {
            BOOL bCondition;
            int i;
            if (m_pICurrentElem || m_bMoreTSReaderOrWriter)
            {
                if (m_bMoreTSReaderOrWriter)
                {
                    m_bMoreTSReaderOrWriter = callbacks.invoke_n_ts(5, 5);
                }

                for (i = 0; i < 10 && m_pICurrentElem; i++)
                {
                    m_pICurrentElem = ischedule.execute_element(m_pICurrentElem);
                    *g_pISchedulerElems += 1;
                }

                if (m_pICurrentElem == NULL && m_bForceSelect == FALSE)
                {
                    GETTIMEOFDAY(now);

                    if (last_left_select_time < now - 0.333)
                    {
                        *g_pForcedSelects += 1;
                        *g_pIsForcedSelect = now.tv_sec;
                        m_bForceSelect = TRUE;
                        if (proc->pc->process_type == PTStreamer)
                        {
                            proc->pc->loadinfo->StreamerForceSelect();
                        }
                    }
                    else
                    {
                        m_CrashState = IN_ISERVERPQ;
                        m_pICurrentElem = ischedule.get_execute_list(now);
                        m_CrashState = IN_LOOP;
                    }
                }

                bCondition = ((m_pSCurrentElem || m_bMoreReaderOrWriter) &&
                    HXMutexTryLock(g_pServerMainLock));
            }
            else if (m_pSCurrentElem || m_bMoreReaderOrWriter)
            {
                bCondition = TRUE;
                HXMutexLock(g_pServerMainLock, TRUE);
            }
            if (bCondition)
            {
                m_bMutexProtection = TRUE;
                for (i = 0; i < 5 && m_pSCurrentElem; i++)
                {
                    m_pSCurrentElem = schedule.execute_locked_element(m_pSCurrentElem);
                    *g_pSchedulerElems += 1;
                }

                if (m_bMoreReaderOrWriter)
                {
                    m_bMoreReaderOrWriter = callbacks.invoke_n(
                        5, 5, g_pMutexNetReaders, g_pMutexNetWriters);
                }
restart_dispatchq_after_GPF:
                m_bMutexProtection = FALSE;
                HXMutexUnlock(g_pServerMainLock);
            }
            if (m_bOnce)
            {
                m_CrashState = IN_DISPATCHQ;
                proc->pc->dispatchq->execute(proc);
                m_CrashState = IN_LOOP;
                m_bOnce = FALSE;
            }
        }
        m_CrashState = NONE;

        g_pMainLoops[procnum]++;

#ifdef _UNIX
        if (g_bReconfigServer)
        {
            if (proc->pc->server_control)
            {
                g_bReconfigServer = FALSE;
                HUPResponse* pResp = new HUPResponse();
                pResp->AddRef();
                proc->pc->server_control->
                    ReconfigServer(pResp);
                pResp->Release();
            }
        }

        if (g_bResetLeaks)
        {
            g_bResetLeaks = FALSE;
            SharedMemory::resetleaks();
        }

        if (g_bCheckLeaks)
        {
            g_bCheckLeaks = FALSE;
            SharedMemory::checkleaks();
        }
#endif

#if defined PAULM_REPORT
        if (g_DoSocketTimes)
        {
#if defined PAULM_SOCKTIMING
            g_SocketTimer.Report();
#endif

#ifdef PAULM_CLIENTTIMING
            g_ClientTimer.Report();
#endif
#ifdef PAULM_SESSIONTIMING
            g_SessionTimer.Report();
#endif
#ifdef PAULM_CHPTIMING
            g_CHPTimer.Report();
#endif
#ifdef PAULM_RTSPPROTTIMING
            g_RTSPProtTimer.Report();
#endif
#ifdef PAULM_CSPTIMING
            g_CSPTimer.Report();
#endif
#ifdef PAULM_NCPTIMING
            g_NCPTimer.Report();
#endif
#ifdef PAULM_TCPSCTIMING
            g_TCPSCTimer.Report();
#endif
#ifdef PAULM_CSTCPSCTIMING
            g_CSTCPSCTimer.Report();
#endif
#ifdef PAULM_STORERECTIMING
            g_STORERECTimer.Report();
#endif
#ifdef PAULM_RTSPPTIMING
            g_RTSPPTimer.Report();
#endif
#ifdef PAULM_RTSPSTTIMING
            g_RTSPSTTimer.Report();
#endif
#ifdef PAULM_INTCPCTIMING
            g_INTCPCTimer.Report();
#endif
#ifdef PAULM_PLAYERTIMING
            g_PlayerTimer.Report();
#endif
#ifdef PAULM_STREAMDATATIMING
            g_StreamDataTimer.Report();
#endif
#ifdef PAULM_PPMTIMING
            g_PPMTimer.Report();
#endif
#ifdef PAULM_TNGTCPTRANSTIMING
            g_TNGTCPTransTimer.Report();
#endif
#ifdef PAULM_TCPSC_LOSTADDREF
            dumpLostAddrefs();
#endif
#ifdef PAULM_HXBUFFERSPACE
            printf("CHXBufferSpace: %lu\n",
                GetBufferDataSize());
#endif
#ifdef PAULM_HXSTRINGSPACE
            printf("CHXStringSpace: %lu\n",
                GetStringDataSize());
#endif
            g_DoSocketTimes = 0;
        }
#endif
    }
}
