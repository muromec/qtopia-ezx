/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: server_engine.h,v 1.8 2007/09/20 14:21:55 rdolas Exp $
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

#ifndef _SERVER_ENGINE_H_
#define _SERVER_ENGINE_H_

#include <setjmp.h>

#include "hxtypes.h"
#include "hxcom.h"

#include "hxcomm.h"
#include "ihxpckts.h"
#include "debug.h"
#include "proc.h"
#include "regdb_misc.h"
#include "hxspriv.h"
#include "engine.h"
#include "engine_desc_reg.h"
#include "streamer_info.h"

class Process;

struct PurgeStruct
{
    IHXStatistics* m_pStats;
    IHXBuffer* m_pBuf;
};

#if defined _UNIX && defined _SOLARIS26
#define HX_SELECT
#endif

#ifdef _UNIX
#define GETTIMEOFDAY(t) ( (*g_bITimerAvailable) ? t = *g_pNow : gettimeofday(&t, 0) )
#else
#define GETTIMEOFDAY(t) gettimeofday(&t, 0);    \
                t.tv_usec = (t.tv_usec / 1000) * 1000;
#endif

#ifdef HX_SELECT

#include <poll.h>

#ifdef _LINUX
#if defined(_NEED_FDS_BITS_DEFINE)
//#define fds_bits __fds_bits // for struct __fd_set member
#endif
typedef int nfds_t;
typedef long fds_mask;
#define FD_NFDBITS NFDBITS
#define HX_EVENT_READ POLLIN
#define HX_EVENT_WRITE POLLOUT
#define HX_EVENT_ERROR POLLPRI
#define HX_REVENT_READ (POLLIN | POLLHUP | POLLERR)
#define HX_REVENT_WRITE (POLLOUT | POLLHUP | POLLERR)
#define HX_REVENT_ERROR POLLPRI
#endif /* _LINUX */

#ifndef _LINUX
/* bits to set when invoking poll  */
#define HX_EVENT_READ   POLLRDNORM
#define HX_EVENT_WRITE  POLLWRNORM
#define HX_EVENT_ERROR  (POLLRDBAND|POLLPRI)
                                /* bits to check when poll returns */
#define HX_REVENT_READ  (POLLRDNORM|POLLHUP|POLLERR)
#define HX_REVENT_WRITE (POLLWRNORM|POLLHUP|POLLERR)
#define HX_REVENT_ERROR (POLLRDBAND|POLLPRI)
#endif /* !_LINUX */

#define HX_TIMEVAL_MAXSEC   100000000  /* maximum number of seconds */
#define HX_TIMEVAL_MAXUSEC  1000000    /* maximum number of micro sec */
#define HX_MIN_POLL_SIZE    1024       /* minimum pollfd size */
#define HX_MAX_POLL_SIZE    16384      /* maximum pollfd size */

typedef struct {
    struct pollfd* pfds;        /* poll struct's */
    int size;           /* current number of of poll struct */
} poll_info;
#endif
const UINT32 MAX_PURGE_ARRAY_SIZE = 32000;

class ServerEngine  : public Engine
                    , public ServerEngineDescReg
{
public:
                            ServerEngine(Process* _proc);
                            ~ServerEngine();
        void                preevent();
        void                postevent();
        void                Dispatch();
        void                RegisterDesc();
        void                UnRegisterDesc();
        void                RegisterSock();
        void                UnRegisterSock();
#ifdef HX_SELECT
        int grow_poll(int nfds);        /* grow pollfd  */
        int                 select(int nfds, fd_set* readfds, fd_set* writefds,
                                fd_set* errorfds, struct timeval* timeout);
#endif
        void                mainloop(BOOL bJumpStart = FALSE);
#ifdef _UNIX
        void                CrashRecover() { siglongjmp(m_JumpBuffer, 1); }
#else
        void                CrashRecover() { longjmp(m_JumpBuffer, 1); }
#endif // _UNIX

#if defined _LINUX || defined _SOLARIS
        void                JumpStart() { siglongjmp(m_JumpStartJumpBuffer, 1); }
#endif // _LINUX || defined _SOLARIS

#ifdef _UNIX
        void                HandleBadFds() {};
#endif
        void*               get_proc(){return (void*)proc;}

    BOOL                    m_bMutexProtection;

private:

    void                    OnSocketsChanged();
    void                    OnDescriptorsChanged();

    Process*            proc;

#ifdef HX_SELECT
    poll_info           m_poll_info;
#endif

    ServPQElem* m_pICurrentElem;
    ServPQElem* m_pSCurrentElem;
    BOOL m_bMoreReaderOrWriter;
    BOOL m_bMoreTSReaderOrWriter;
    BOOL m_bOnce;
    BOOL m_bForceSelect;

#ifdef _UNIX
    sigjmp_buf m_JumpBuffer;
#else
    jmp_buf m_JumpBuffer;
#endif

#if defined _LINUX || defined _SOLARIS
    sigjmp_buf m_JumpStartJumpBuffer;
#endif // defined _LINUX || defined _SOLARIS

    enum _CrashState { IN_LOOP, IN_DISPATCHQ, IN_SERVERPQ, IN_ISERVERPQ, NONE };
    _CrashState m_CrashState;
};

inline void
ServerEngine::RegisterDesc()
{
    RegisterDescriptors(1);
}

inline void
ServerEngine::UnRegisterDesc()
{
    UnRegisterDescriptors(1);
}

inline void
ServerEngine::RegisterSock()
{
    RegisterSockets(1);
}

inline void
ServerEngine::UnRegisterSock()
{
    UnRegisterSockets(1);
}


inline void
ServerEngine::OnSocketsChanged()
{
#ifdef _WIN32
    if (proc->pc)
        proc->pc->streamer_info->SetImportantThingCount(proc, m_ulSockCount);
#else
    if (proc->pc)
        proc->pc->streamer_info->SetImportantThingCount(proc, m_ulDescCount);
#endif
}

inline void
ServerEngine::OnDescriptorsChanged()
{
#ifdef _UNIX
    if (proc->pc)
        proc->pc->streamer_info->SetImportantThingCount(proc, m_ulDescCount);
#endif
}

inline
ServerEngine::~ServerEngine()
{
#ifdef HX_SELECT
    if (m_poll_info.pfds)
    {
        HX_VECTOR_DELETE(m_poll_info.pfds);
    }
#endif
}

#endif /* _SERVER_ENGINE_H_ */
