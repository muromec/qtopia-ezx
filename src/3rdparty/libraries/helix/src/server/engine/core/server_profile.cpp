/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_profile.cpp,v 1.11 2005/10/22 01:59:11 dcollins Exp $ 
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
 *  The Routines defined here implement the profiling calls to enable 
 *  profiling start, suspension and dumping of profile data collection.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#if !defined(_WIN32)

#include <unistd.h>
#include <signal.h>

#ifdef _HPUX
#include <sys/signal.h>
#endif

#endif

#include "_main.h"
#include "hxcom.h"
#include "mutex.h"
#include "debug.h"
#include "server_version.h"

#define RESETPROFILING_SIGNAL SIGVTALRM
#define DUMPPROFILING_SIGNAL SIGWINCH
#define SUSPENDPROFILING_SIGNAL SIGILL
//#define RESETPROFILING_SIGNAL SIGRTMIN
//#define DUMPPROFILING_SIGNAL (SIGRTMIN+1)
//#define SUSPENDPROFILING_SIGNAL (SIGRTMIN+2)

enum eProfilingState {PROF_INIT, PROF_STARTED, PROF_SUSPENDED, PROF_STOPPED};
enum eEnableProfiling {PROF_ENABLE, PROF_DISABLE};

HX_MUTEX g_pProfilingLock;
eProfilingState* g_pProfilingState = 0;


/***********************************************************************
 *  FreeBSD / OpenBSD / NetBSD
 *  Requires a statically-linked server
 ***********************************************************************/
#if defined(_FREEBSD) || defined(_OPENBSD) || defined(_NETBSD)

#define USE_GPROF
#define PROF_PTR u_long *
extern "C" void* _start;
extern "C" void* _fini;
extern "C" void* _etext;
extern "C" void* __rec_iput;
#define PROF_START &_start
//#define PROF_END   0x0827d994 //XXXDC FIXME
#define PROF_END   &__rec_iput //XXXDC FIXME

extern "C" void monstartup(PROF_PTR lowpc, PROF_PTR highpc);
extern "C" void moncontrol(int mode);
extern "C" int _mcleanup(void);


/***********************************************************************
 *  Tru64
 ***********************************************************************/
#elif defined _OSF1

#include <mon.h>
//void monitor_signal(int signal);
// Be sure to use the PROFDIR and PROFFLAGS environment variables on 
// this platform (see the gprof manpage).
extern "C" void* main;
extern "C" void* etext;
#define PROF_PTR caddr_t
#define PROF_START &main
#define PROF_END   &etext //XXXDC FIXME

void
ProfileDumpData()
{
    monitor_signal(SIGPROF);
};


/***********************************************************************
 *  Linux w/ Intel VTune
 ***********************************************************************/
#elif defined _LINUX && defined USE_VTUNE

#include "VtuneApi.h"

void
ProfileInit()
{
    // no init necessary
}

void
ProfileDumpData()
{
    //XXX HOW?
    // This does not appear to be possible, dump results using
    // external VTune commands.
}

void
ProfileControl(eEnableProfiling n)
{
    if (n == PROF_ENABLE)
    {
        VTResume();
    }
    else
    {
        VTPause();
    }
}

void
ProfileCleanup()
{
    // no cleanup necessary
}


/***********************************************************************
 *  Linux
 *  Requires a statically-linked server
 ***********************************************************************/
#elif defined _LINUX

#define USE_GPROF
#include <sys/gmon.h>
#define PROF_PTR u_long
extern "C" void _start(void);
extern "C" void etext(void);
#define PROF_START &_start
#define PROF_END &etext 

extern "C" void monstartup(PROF_PTR lowpc, PROF_PTR highpc);
extern "C" void moncontrol(int mode);


/***********************************************************************
 *  Solaris + gprof
 ***********************************************************************/
//#elif 0 //force use of libcollector.so
#elif defined _SOLARIS

#define USE_GPROF
#define PROF_PTR char*
extern "C" void _start(void);
extern "C" void etext(void);
extern "C" void _init(void);
#define PROF_START &_start
#define PROF_END (&etext ? &etext : &_init)
extern "C" void _mcleanup(void);
extern "C" void monstartup(PROF_PTR lowpc, PROF_PTR highpc);
extern "C" void moncontrol(int mode);
//#endif


/***********************************************************************
 *  Solaris + libcollector.so
 ***********************************************************************/
#elif 0 // doesn't work right yet
//#elif defined _SOLARIS

#include "libcollector.h"

#define PROF_PTR char*

extern "C" void _start(void);
extern "C" void etext(void);
extern "C" void _init(void);

#define PROF_START &_start
#define PROF_END (&etext ? &etext : &_init)

void
ProfileInit()
{
    char szName[64];
    sprintf(szName, "%s.%d", ServerVersion::ExecutableName(), getpid());
    collector_sample(szName);
}

void
ProfileDumpData()
{
    collector_terminate_expt();
    printf ("PROFILING: collector_terminate_expt returned [PID=%d]\n",
	    getpid());
    fflush(stdout);
};

void
ProfileControl(eEnableProfiling n)
{
    if (n == PROF_ENABLE)
    {
        collector_resume();
    }
    else
    {
        collector_pause();
    }
}

void
ProfileCleanup()
{
}

/***********************************************************************
 *  Other
 ***********************************************************************/
#else

#error unsupported platform

#endif


/************************************************************************
 * Generic Gprof-related code
 ************************************************************************/
#if defined USE_GPROF

void
ProfileInit()
{
    PROF_PTR pFirst = (PROF_PTR) PROF_START;
    PROF_PTR pLast  = (PROF_PTR) PROF_END;
    monstartup(pFirst, pLast);
    moncontrol(0);
}

void
ProfileControl(int n)
{
    if (n == PROF_ENABLE)
    {
        moncontrol(1);
    }
    else
    {
        moncontrol(0);
    }
}

#ifndef _OSF1
void
ProfileDumpData()
{
    _mcleanup();
    printf ("PROFILING: _mcleanup returned [PID=%d]\n", getpid());
    fflush(stdout);
};
#endif

void
ProfileCleanup()
{
    // we don't want the gmon.out files from multiple processes
    // to overwrite each other...
    char szNewFile[256];
    char szFile1[256];
    char szFile2[256];
    sprintf (szNewFile, "%s.%d.gmon", ServerVersion::ExecutableName(),
             getpid());
    sprintf (szFile1, "%s.gmon", ServerVersion::ExecutableName());
    sprintf (szFile2, "%s.gmon", progname);
  
    if (rename (szFile1, szNewFile) == 0)
    {
        printf ("renamed %s to %s\n", szFile1, szNewFile);
    }
    else if (rename (szFile2, szNewFile)  == 0)
    {
        printf ("renamed %s to %s\n", szFile2, szNewFile);
    }
    else if (rename ("gmon.out", szNewFile)  == 0)
    {
        printf ("renamed gmon.out to %s\n", szNewFile);
    }
    else
    {
        printf ("Unable to rename gmon.out file\n");
    }
}

#endif


/************************************************************************/
/************************************************************************/

void ProfileReset(int code);
void ProfileSuspend(int code);
void ProfileDump(int code);


/***********************************************************************
 *  ProfilingSetup
 */
void
ProfilingSetup(void)
{
    HXMutexLock(g_pProfilingLock, TRUE);

    if (!g_pProfilingState)
    {
        g_pProfilingState = new eProfilingState;
    }

    ProfileInit();

    *g_pProfilingState = PROF_INIT;
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;

    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))ProfileReset;
    sigaction(RESETPROFILING_SIGNAL, &sa, NULL);

    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))ProfileSuspend;
    sigaction(SUSPENDPROFILING_SIGNAL, &sa, NULL);

    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))ProfileDump;
    sigaction(DUMPPROFILING_SIGNAL, &sa, NULL);

#if defined PROF_START && defined PROF_END
    printf ("PROFILING PID %d: SIGVTALRM=start SIGILL=suspend "
            "SIGWINCH=suspend+dump addrs=[%p:%p]\n",
            getpid(), PROF_START, PROF_END);
#else
    printf ("PROFILING PID %d: SIGVTALRM=start SIGILL=suspend "
            "SIGWINCH=suspend+dump\n", getpid());
#endif
    //printf ("PROFILING PID %d: SIGRTMIN(%d)=start SIGRTMIN+2(%d)=suspend "
    //        "SIGRTMIN+1(%d)=suspend+dump addrs=[%p:%p]\n",
    //        getpid(), SIGRTMIN, SIGRTMIN+2, SIGRTMIN+1, PROF_START, PROF_END);
    fflush(0);

    HXMutexUnlock(g_pProfilingLock);
}

/***********************************************************************
 *  ProfileReset
 */
void
ProfileReset(int code)
{
    HXMutexLock(g_pProfilingLock, TRUE);
    printf ("PROFILING: start profiling for PID=%d\n", getpid());
    fflush(stdout);
  
    if ((*g_pProfilingState) == PROF_STOPPED)
    {
        ProfileInit();
        *g_pProfilingState = PROF_STARTED;
    } 
    else
    {
        ProfileControl(PROF_ENABLE);  // start profiling
        *g_pProfilingState = PROF_STARTED;
    }
    HXMutexUnlock(g_pProfilingLock);
}

/***********************************************************************
 *  ProfileSuspend
 */
void
ProfileSuspend(int code)
{
    HXMutexLock(g_pProfilingLock, TRUE);
    ProfileControl(PROF_DISABLE); // suspend profiling
    *g_pProfilingState = PROF_SUSPENDED;    
    printf ("PROFILING: suspend profiling for PID=%d\n", getpid());
    HXMutexUnlock(g_pProfilingLock);
}

/***********************************************************************
 *  ProfileDump
 */
void
ProfileDump(int code)
{
    HXMutexLock(g_pProfilingLock, TRUE);

    ProfileControl(PROF_DISABLE);  // suspend profiling  
    if (*g_pProfilingState != PROF_STOPPED)
    {
        ProfileDumpData();
        *g_pProfilingState = PROF_STOPPED;
        ProfileCleanup();
    }
    else
    {
        printf ("PROFILING: profiling already stopped for PID=%d\n", getpid());
    }
    fflush(0);

    HXMutexUnlock(g_pProfilingLock);
    return;
}
