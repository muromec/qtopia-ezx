/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: _main.cpp,v 1.144 2007/03/05 23:24:05 atin Exp $
 *
 * Portions Copyright (c) 1995-2007 RealNetworks, Inc. All Rights Reserved.
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

#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/time.h"
#include "hlxclib/netdb.h"
#include "hlxclib/sys/types.h"
#include "hlxclib/sys/stat.h"

#if !defined(_WIN32)

#include "hlxclib/unistd.h"
#include "hlxclib/pwd.h"
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#ifdef XXXAAK_SOLARIS_COUNT_PROC_FDS
#include "hlxclib/dirent.h"
#endif

#include <sys/socket.h>
#include "unix_misc.h"

#if defined _SOLARIS
#include <sys/systeminfo.h>
#elif defined _HPUX
#include <sys/signal.h>
#include <sys/param.h>
#endif

#if defined _LINUX  || defined PTHREADS_SUPPORTED
#include <sys/mman.h>
//#define REPORT_STACK_USAGE
size_t get_base_of_stack(size_t sp);
#endif // _LINUX || PTHREADS_SUPPORTED

#ifdef _LINUX
#include <linux/unistd.h>
#include <unistd.h>
#if defined PTHREADS_SUPPORTED
#include <sys/types.h>
#include <linux/unistd.h>
#include <errno.h>
pid_t gettid(void);
#endif // PTHREADS_SUPPORTED
#endif // _LINUX

#endif /* !_WIN32 */

#if defined _WIN32
#include "hlxclib/windows.h"
#include <process.h>
#endif

typedef enum { NO_FAULT, IN_FAULT_HANDLER, IN_FAULT_DOUBLE_FAULT_HANDLER} CrashStateType;

// thread local storage of procnum
#ifdef _WIN32
__declspec(thread) int g_nTLSProcnum = -1;
__declspec(thread) CrashStateType g_eTLSCrashState = NO_FAULT;
#elif ((defined _LINUX && LINUX_EPOLL_SUPPORT) || (defined _SOLARIS && defined DEV_POLL_SUPPORT)) && defined PTHREADS_SUPPORTED
__thread int g_nTLSProcnum = -1;
__thread CrashStateType g_eTLSCrashState = NO_FAULT;
__thread UINT32 g_ulTLSNumTripleFaults = 0;
#else
int g_nTLSProcnum = -1;
CrashStateType g_eTLSCrashState = NO_FAULT;
UINT32 g_ulTLSNumTripleFaults = 0;
#endif // _WIN32

#define MAX_NUM_TRIPLE_FAULTS 10

#include "hxcom.h"
#include "hxerror.h"
#include "hxmap.h"
#include "hxslist.h"
#include "hxstring.h"
#include "plgnhand.h"
#include "dllpath.h"
#include "proc.h"
#include "shmem.h"
#include "time.h"
#include "mutex.h"
#include "simple_callback.h"
#include "dispatchq.h"
#include "core_proc.h"
#include "hxcfg.h"
#include "controller_container.h"
#include "config.h"
#include "fdpass_socket.h"
#include "streamer_proc.h"
#include "resolver_proc.h"
#include "misc_proc.h"
#include "memreap_proc.h"
#include "server_engine.h"
#include "server_context.h"
#include "server_inetwork.h"
#include "server_resource.h"
#include "acceptor.h"
#include "base_errmsg.h"
#include "_main.h"
#include "trace.h"
#include "microsleep.h"
#include "hxtime.h"
#include "buildinfo.h"
#include "server_version.h"
#include "multicast_mgr.h"
#include "server_info.h"
#include "defslice.h"
#include "hxtick.h"
#include "hxmon.h"
#include "hxreg.h"
#include "loadinfo.h"
#include "platform.h"
#include "globals.h"
#include "load_balanced_listener.h"
#include "lbl_cdispatch.h"
#include "mem_cache.h"
#include "fastfile_stats.h"
#include "xmlconfig.h"
#include "clientguid.h"
#include "sysinfo.h"
#include "logoutputs.h"
#include "hxlogoutputs.h"
#include "rssmgr.h"
#include "rtspstats.h"
#include "sdpstats.h"
#include "shutdown.h"
#include "osdetect.h"

#define SELF_DEFINE_RESTART_SIGNAL 255

// Forward declare some static members...
UINT32 RSSManager::m_ulRSSReportInterval;
BOOL RSSManager::m_bRSSReportEnabled;
LogFileOutput* RSSManager::m_pOutput;

#define HX_COPYRIGHT_INFO "(c) 1995-2007 RealNetworks, Inc. All rights reserved."

#define MAX_STARTUPLOG_LEN 1024
char g_szStartupLog[MAX_STARTUPLOG_LEN];
BOOL g_bUseStartupLog = FALSE;
static BOOL bRestartOnFault = FALSE;

#ifdef _LINUX
#include <dlfcn.h> // for loading gdbcomp library

// 2.0.x kernels (RH5.2 etc) used SA_STACK
#if defined(SA_STACK) && !defined(SA_ONSTACK)
#define SA_ONSTACK SA_STACK
#endif

#endif /* _LINUX */

#ifdef PAULM_LEAKCHECK
/*
 * To use this, define PAULM_LEAKCHECK in core_proc.h
 */
UINT32 g_ulLeakCheckFirst;
UINT32 g_ulLeakCheckAlloc;
UINT32 g_ulLeakCheckFree;
#endif
BOOL g_bLeakCheck = FALSE;

BOOL g_bStopAliveChecker = FALSE;

#ifdef SHARED_FD_SUPPORT
extern int* g_pProcessPipes[MAX_THREADS];
#endif // SHARED_FD_SUPPORT

BOOL* g_bMmapGrowsDown;
#if defined _LINUX || defined PTHREADS_SUPPORTED
HX_MUTEX g_pAllocateStackMutex;
char* controllerBOS = 0;
char* maxThreadStack = 0;

#ifdef _SOLARIS
char* nextThreadStack = 0;
#endif //_SOLARIS && PTHREADS_SUPPORTED

typedef struct StackUsage
{
    char* pStack;
    UINT32 ulUsage;
} Stackusage;

StackUsage g_threadStacks[MAX_THREADS];

char* g_pStackBase = 0;

struct descr_struct
{
    int pid;
};

struct handle_struct
{
    descr_struct* descr;
};
volatile int* g_pPthreadsDebug = 0;
volatile handle_struct* g_pPthreadHandles = 0;
volatile int g_nRestartSignal;
volatile int g_nCancelSignal;
volatile int g_nDebugSignal = 0;
volatile int* g_pPthreadHandlesNum;
BOOL g_bWaitingForRestartSignal = FALSE;
sigjmp_buf g_restartBuf;

void
handle_sigcancel(int)
{
}

void
handle_sigrestart(int)
{
    // do nothing unless we're explicitly waiting for this signal
    if (g_bWaitingForRestartSignal)
        siglongjmp(g_restartBuf, 1);
}

void
handle_sigdebug(int)
{
}

void
WaitForRestartSignal()
{
    sigset_t mask;

    if (!g_pPthreadsDebug || !*g_pPthreadsDebug || !g_nDebugSignal)
        return;

    g_bWaitingForRestartSignal = TRUE;
    if (sigsetjmp(g_restartBuf, 0))
    {
        // we jumped here so we've been restarted
        g_bWaitingForRestartSignal = FALSE;
        return;
    }

    // signal gdb that we're suspending
    kill(getpid(), g_nDebugSignal);

    // block all signals except restart
    sigfillset(&mask);
    sigdelset(&mask, g_nRestartSignal);

    //wait for that signal
    sigsuspend(&mask);
}
#endif /* _LINUX || PTHREADS_SUPPORTED */


#include "sapclass.h"
#ifdef LINUX_STACK_FENCE_POST
#include <unistd.h>
#include <sys/mman.h>
#endif

#ifdef _LINUX
#include "resolver_mux.h"
#endif

#include "sockpair.h"
#include "servsked.h"


ENABLE_MULTILOAD_DLLACCESS_PATHS(Server);

#ifdef _UNIX
static int          MonitorControllerSocket[2] = { -1, -1 };
#endif

SocketPair MonitorAliveChecker;

SharedMemory*   SharedMemory::self      = 0;
float g_AllocCoefficient = 1.1F;

int* VOLATILE   Process::_numprocs      = 0;
PROCID_TYPE* VOLATILE   Process::_procid;
ServerEngine** VOLATILE Process::_pserver_engines = 0;
Engine** volatile g_ppLastEngine;
UINT32* volatile g_pDescriptorCapacityValue = 0;
UINT32* volatile g_pSockCapacityValue = 0;
BOOL             g_bNoAutoRestart        = FALSE;
BOOL             g_bForceRestart         = FALSE;
BOOL g_bUnsupportedOSFlag = FALSE;

HX_MUTEX g_pServerMainLock;

#ifdef ENABLE_PROFILING
extern HX_MUTEX g_pProfilingLock;
void ProfilingSetup();
#endif

HX_MUTEX g_pCrashPrintLock;

#ifdef _WIN32
UINT32* VOLATILE Process::_ptids = 0;
HANDLE* VOLATILE Process::_ptandles = 0;
unsigned long g_last_known_eip = 0;
unsigned long g_last_known_ebp = 0;

void DumpThreadState();
#endif
#ifdef _UNIX
INT32           g_ControllerPid         = 0;
int             g_MonitorPID = 0;
int             g_AliveCheckerPID = 0;
int             g_GlobalProcessList[MAX_THREADS];
char*           g_GlobalProcListFiles[MAX_THREADS];
BOOL* volatile  g_pbHandleCHLD          = 0;
BOOL* volatile  g_pbRestarting          = 0;

#if defined PTHREADS_SUPPORTED && defined _LINUX
// This list contains only the ids of threads that are created by controller.
// This list is used to send pthread_kill() signals to all the threads during
// HBF to print the stack traces.
unsigned long* g_GlobalThreadList = NULL;
#endif // defined PTHREADS_SUPPORTED && defined _LINUX

#ifdef DEV_POLL_SUPPORT
BOOL g_bUseDevPoll = TRUE;
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
BOOL g_bUseEPoll = TRUE;
#endif // DEV_POLL_SUPPORT

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif

#ifdef PTHREADS_SUPPORTED
pthread_t g_ControllerTid = -1;
BOOL g_bUnixThreads = TRUE;
#endif // PTHREADS_SUPPORTED

extern BOOL g_bDisableInternalResolver;

extern "C" void
AddToProcessList(int pid)
{
    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (g_GlobalProcessList[i] == 0)
        {
            g_GlobalProcessList[i] = pid;
            g_GlobalProcListFiles[i] = new char[1024];
#if defined _LINUX
            sprintf(g_GlobalProcListFiles[i], "/proc/%d/stat", pid);
#elif defined _FREEBSD || defined _OPENBSD || defined _NETBSD
            sprintf(g_GlobalProcListFiles[i], "/proc/%d/status", pid);
#else
            sprintf(g_GlobalProcListFiles[i], "/proc/%d", pid);
#endif
            return;
        }
    }
}

#if defined PTHREADS_SUPPORTED && defined _LINUX
extern "C" void
AddToThreadList(unsigned long tid)
{
    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (g_GlobalThreadList[i] == 0)
        {
            g_GlobalThreadList[i] = tid;
            return;
        }
    }
}
#endif // defined PTHREADS_SUPPORTED && defined _LINUX

BOOL
IAmController()
{
#ifdef PTHREADS_SUPPORTED
    if (g_bUnixThreads)
        return getpid() == g_ControllerPid && pthread_self() == g_ControllerTid;
#endif // PTHREADS_SUPPORTED

    return (getpid() == g_ControllerPid);
}

BOOL
IAmMonitor()
{
    return getpid() == g_MonitorPID;
}

BOOL
IAmAliveChecker()
{
    return getpid() == g_AliveCheckerPID;
}


void*           pMakeProcess = 0;
#endif

#ifdef PAULM_HXBUFFERSPACE
UINT32* volatile g_pCHXBufferTotalData = 0;
#endif

#ifdef PAULM_HXSTRINGSPACE
UINT32* volatile g_pCHXStringTotalData = 0;
#endif

#ifdef PAULM_ALLOCDUMP
extern void flush_alloc();
#endif

#ifdef _EFENCE_ALLOC
int g_backfence_alloc = 0;
int g_efence_alloc = 0;
BOOL g_bEFenceProtectFreedMemory = FALSE;
#endif

#define CP_CREATE_SOCKET              0x1
#define CP_FORK_PROCESS               0x2

static const int zMaxOpenFDs = 65536;

DispatchQueue*   g_dq;

#ifdef _LINUX
    UINT32 g_ulLinuxNoSegvSyscall = 0;
#ifndef __NR_get_segv_eip
#define __NR_get_segv_eip 240
#endif
    _syscall1(int, get_segv_eip, void**, peip);
#endif

#if defined PTHREADS_SUPPORTED || defined _LINUX
UINT32 g_ulThreadStackSize = 1024 * 1024;
#endif // defined PTHREADS_SUPPORTED || defined _LINUX

#ifdef SHARED_FD_SUPPORT
#  if defined(_LINUX) && !defined(PTHREADS_SUPPORTED)
    BOOL g_bSharedDescriptors = FALSE;
#  else
    BOOL g_bSharedDescriptors = TRUE;
#  endif // _LINUX && !PTHREADS_SUPPORTED
#endif // SHARED_FD_SUPPORT

// 0 means available but off by default
#ifndef FAST_MALLOC_ALL
#define FAST_MALLOC_ALL 1
#endif // FAST_MALLOC_ALL

BOOL g_bSlightAcceleration = FALSE;
BOOL g_bNoCrashAvoidance = 0;
BOOL g_bCrashAvoidancePrint = 0;
BOOL g_bPrintProcessPIDs = 0;
BOOL g_bNoClientValidation = 0;
BOOL g_bReportRestart = 0;
BOOL g_bFastMalloc = TRUE;
BOOL g_bFastMallocAll = FAST_MALLOC_ALL;
BOOL g_bForceThreadSafePlugins = FALSE;
BOOL g_bSkipCPUTest = FALSE;
BOOL g_bAllowCoreDump = FALSE;
BOOL g_bShowDebugErrorMessages = FALSE;
BOOL g_bDisableHeartbeat = FALSE;
char* g_pHeartBeatIP = 0;
BOOL g_bDisablePacketAggregation = FALSE;
BOOL g_bIgnoreEtcHosts = FALSE;
BOOL g_bDisableFastClock = FALSE;
BOOL g_bOgreDebug = FALSE;
UINT32 g_ulDebugFlags = 0;
UINT32 g_ulDebugFuncFlags = 0;
INT32 g_ulCoreCDistDebugFlags = 0;
char* VOLATILE g_pszConfigFile = 0;
char* VOLATILE g_pszImportKey = 0;
BOOL  g_bQueryMSS = FALSE;

UINT32 g_ulBackOffPercentage = 10; // default
UINT32 g_ulSizemmap = 0;

UINT32 g_ulMaxCrashAvoids = 0;

UINT32* g_pBytesServed = 0;
UINT32* g_pPPS = 0;
UINT32* g_pLiveIncomingPPS = 0;
UINT32* g_pOverloads = 0;
UINT32* g_pBehind = 0;
UINT32* g_pNoBufs = 0;
UINT32* g_pOtherUDPErrs = 0;
UINT32* g_pMainLoops = 0;
UINT32* g_pForcedSelects = 0;
UINT32* g_pAggregateRequestedBitRate = 0;
UINT32* g_pConcurrentOps = 0;
UINT32* g_pConcurrentMemOps = 0;
UINT32* g_pSchedulerElems = 0;
UINT32* g_pISchedulerElems = 0;
UINT32* g_pTotalNetReaders = 0;
UINT32* g_pTotalNetWriters = 0;
UINT32* g_pMutexNetReaders = 0;
UINT32* g_pMutexNetWriters = 0;
UINT32* g_pIdlePPMs = 0;
UINT32* g_pWouldBlockCount = 0;
UINT32* g_pSocketAcceptCount = 0;
UINT32* g_pAggregatablePlayers = 0;
UINT32* g_pFileObjs = 0;
UINT32* g_pCPUCount = 0;
UINT32* g_pNumStreamers = 0;
UINT32* g_pResends = 0;
UINT32* g_pAggreg = 0;
UINT32* g_pIsForcedSelect = 0;
Timeval* g_pNow = 0;
BOOL*   g_bITimerAvailable = 0;
UINT32* g_pNumCrashAvoids = 0;
UINT32* g_pNumCrashAvoidsInWindow = 0;
Timeval* g_pCrashAvoidWindowTime = 0;
BOOL*   g_bLimitParallelism = 0;
UINT32* g_pMemoryMappedDataSize = 0;
UINT32* g_pAggregateTo = 0;
UINT32* g_pAggregateHighest = 0;
UINT32* g_pAggregateMaxBPS = 0;
UINT32* g_pStreamerCount = 0;

BOOL* g_pProcessStartInProgress = 0;

#if defined(_LINUX) && !defined(PTHREADS_SUPPORTED)
// This is used by platform/unix/pthread_compat.cpp, not by regular pthreads
extern CHXSimpleList *g_PThreadMutexCleanupStack[MAX_THREADS];
#endif //defined(_LINUX) && !defined(PTHREADS_SUPPORTED)

// System-specific information Generator
CSysInfo* g_pSysInfo = NULL;

// NG statistics
double* g_pBroadcastDistBytes = 0;
UINT32* g_pBroadcastDistPackets = 0;
double* g_pBroadcastRecvBytes = 0;
UINT32* g_pBroadcastRecvPackets = 0;
UINT32* g_pBroadcastPacketsDropped = 0;
UINT32* g_pBroadcastPPMOverflows = 0;

UINT32* g_pSizes = 0;
BOOL g_bDoSizes = FALSE;
extern BOOL g_bReportHXCreateInstancePointer;
extern UINT32* g_pNAKCounter;

BOOL* volatile g_pbTrackAllocs;
BOOL* volatile g_pbTrackFrees;

PluginHandler*  g_plugin_handler = NULL;
ClientGUIDTable* g_pClientGUIDTable = NULL;

HXProtocolManager* g_pProtMgr = NULL;

extern BOOL* g_pNewConnAllow;
extern UINT32* g_pSecondsToShutdown;
extern UINT32* g_pClientTerminationBySelf;
extern UINT32* g_pClientTerminationForced;
extern UINT32* g_pClientIgnoreTermReq;
extern BOOL*   g_bEnableClientConnCleanup;

UINT32* g_pSignalCount;
#if defined __GNUC__

#ifndef _SOLARIS
#include <new.h>
#endif

typedef void (*vfp)(void);
void __my_new_handler (void);

void
__my_new_handler ()
{
    printf("Trapped Fault (m/43): Please File Bug Report!\n");
    volatile int* x = 0;
    *x = 5;
}
#endif

void procstr(char *src);

int shared_ready = 0;
void* g_pSecondHeapBottom = 0;
#ifdef _LINUX
void* g_pSbrkHeapBottom = 0;
void* g_pSbrkHeapTop = 0;
void* g_pDataSegmentBottom;
#endif // _LINUX

int* VOLATILE pMiscProcInitMutex;
int* VOLATILE pTimerProcInitMutex;
int* VOLATILE pResolverProcInitMutex;
int* VOLATILE pStreamerProcInitMutex;
int* VOLATILE pHXProcInitMutex;
int* VOLATILE pMemreapProcInitMutex;


// Add a plugin's description to this list to prevent the server
// from starting without loading it
const char* CoreTransferCallback::zm_pRequiredPlugins[] =
                        {NULL};

// This checksum should be set to -5 times the number of Required Plugins
// in the list, so that we can be sure no one hacked the server binary
const INT16 CoreTransferCallback::zm_nReqPlugChecksum = 0;

extern char * VOLATILE * VOLATILE restartargv;
extern char * VOLATILE * VOLATILE origargv;

int CPUDetect();
int GetStreamerCount(Process* proc);
static BOOL RedHatEtcHostsOkay();
void killme(int code);
void gracefully_killme(int code);

void
GetAggregationLimits(UINT32       ulActualDeliveryRate,
                     UINT32*      pAggregateTo,
                     UINT32*      pAggregateHighest)
{
    // this is not our engine or proc, we're just trying to get to
    // the shared loadstate structure
    Process* pProc =  (Process*)(*g_ppLastEngine)->get_proc();
    _ServerState State = pProc->pc->loadinfo->GetLoadState();

    int i = (int)State;
    if (State == ExtremeLoad || g_pAggregateMaxBPS[i] == 0 ||
        ulActualDeliveryRate < g_pAggregateMaxBPS[i])
    {
        *pAggregateTo = g_pAggregateTo[i];
        *pAggregateHighest = g_pAggregateHighest[i];
    }
    else
    {
        *pAggregateTo = g_pAggregateTo[i+1];
        *pAggregateHighest = g_pAggregateHighest[i+1];
    }
}

void
CoreTransferCallback::func(Process* proc)
{
    char szMsg[80];
    sprintf(szMsg, "%s starting up\n", ServerVersion::ProductName());
    DPRINTF(D_INFO, (szMsg));
    CHXSimpleList::Iterator i;
    CHXSimpleList*          misc_plugins;
    IHXBuffer*              pBuffer = NULL;

#ifdef DEBUG
    debug_level()      = m_debug_level;
    debug_func_level() = m_debug_func_level;
    IHXBuffer* pCDBuf  = NULL;
    proc->pc->registry->GetStr("config.ContentSubscription.DebugLevel",
                               pCDBuf, proc);
    if (pCDBuf)
    {
        g_ulCoreCDistDebugFlags = (INT32) strtol((char*)pCDBuf->GetBuffer(), NULL, 0);
        pCDBuf->Release();
    }
#endif

#if defined _UNIX
    suprintf ("Testing File Descriptors...\n");
    int f[zMaxOpenFDs];
    int index;
    int maxfd;
    int first = -1;
    for (index = 0; index < zMaxOpenFDs; index++)
    {
        f[index] = open("/dev/null", O_RDONLY);
        if (f[index] < 0)
        {
            break;
        }
        else
        {
            if (index == 0)
            {
                first = f[index];
            }
        }
    }

    maxfd = index;

    for (index = 0; index < maxfd; index++)
    {
        close(f[index]);
    }

    UINT32 ulRealMaxFD = maxfd;
    /* Reserve about 1/3 descriptors off the top */
    maxfd = (int)(maxfd * 0.67);

    *g_pDescriptorCapacityValue = maxfd;
    suprintf("Setting per-process descriptor capacity to %ld(%ld), %ld...\n",
        maxfd, ulRealMaxFD, first);
#endif

    // Test Mutex
    // This is just a raw speed test, without excercising mutex
    // contention behavior.
    suprintf("Testing Mutex...");
    fflush(stdout);
    HX_MUTEX pTestLock = HXCreateMutex();
    HXTime time1;
    HXTime time2;
    UINT32 ulLoops = 100000;
    UINT32 ulCurLoop=0;
    gettimeofday(&time1, NULL);
    for (ulCurLoop=0; ulCurLoop < ulLoops; ulCurLoop++)
    {
        HXMutexLock(pTestLock);
        HXMutexUnlock(pTestLock);
    }
    gettimeofday(&time2, NULL);
    HXMutexDestroy(pTestLock);
    float fResult = 0.0f;
    float fUSec = (time2.tv_usec - time1.tv_usec) +
        (time2.tv_sec - time1.tv_sec) * 1000000.0f;
    if (fUSec > 0)
        fResult = 16.0f * ulLoops / fUSec;
    suprintf("(%0.2f ops/usec)\n", fResult);

    // Test HXAtomicOps
    // This is also just a raw speed test with no contention.
//on Solaris w/ gcc with release builds, this gets optimiezd
//in such a way by the compler that it causes a startup hang.
#if !defined(_SOLARIS) || defined(_NATIVE_COMPILER)
    suprintf("Testing AtomicOps...");
    fflush(stdout);
    UINT32 ulTestNum=0;
    INT32 nTestNum=0;
    UINT32 ulTestRet=0;
    ulLoops = 10000;
    gettimeofday(&time1, NULL);
    for (ulCurLoop=0; ulCurLoop < ulLoops; ulCurLoop++)
    {
        HXAtomicIncUINT32(&ulTestNum);
        HXAtomicDecUINT32(&ulTestNum);
        HXAtomicAddUINT32(&ulTestNum, ulCurLoop);
        HXAtomicSubUINT32(&ulTestNum, ulCurLoop);
        HXAtomicIncRetUINT32(&ulTestNum);
        HXAtomicDecRetUINT32(&ulTestNum);
        HXAtomicAddRetUINT32(&ulTestNum, ulCurLoop);
        HXAtomicSubRetUINT32(&ulTestNum, ulCurLoop);
        HXAtomicIncINT32(&nTestNum);
        HXAtomicDecINT32(&nTestNum);
        HXAtomicAddINT32(&nTestNum, (INT32)ulCurLoop);
        HXAtomicSubINT32(&nTestNum, (INT32)ulCurLoop);
        HXAtomicIncRetINT32(&nTestNum);
        HXAtomicDecRetINT32(&nTestNum);
        HXAtomicAddRetINT32(&nTestNum, (INT32)ulCurLoop);
        HXAtomicSubRetINT32(&nTestNum, (INT32)ulCurLoop);
    }
    gettimeofday(&time2, NULL);
    HX_ASSERT(ulTestNum == 0 && nTestNum == 0);
    fResult = 0.0f;
    fUSec = (time2.tv_usec - time1.tv_usec) +
        (time2.tv_sec - time1.tv_sec) * 1000000.0f;
    if (fUSec > 0)
        fResult = 16.0f * ulLoops / fUSec;
    suprintf("(%0.2f ops/usec)\n", fResult);
#endif

    HX_RESULT result;
    char pMessage[2048];

    // Set Plugin directory to the path specified in the config file
    if (HXR_OK == proc->pc->registry->GetStr("config.PluginDirectory",
                                             pBuffer, proc))
    {
        GetDLLAccessPath()->SetPath(DLLTYPE_PLUGIN,
            (const char*)pBuffer->GetBuffer());
        sprintf (pMessage, "Loading Plugins from %s...",
            (const char*)pBuffer->GetBuffer());
    }
    HX_RELEASE(pBuffer);

    // Set Common directory to the path specified in the config file
    if (HXR_OK == proc->pc->registry->GetStr("config.SupportPluginDirectory",
                                             pBuffer, proc))
    {
        GetDLLAccessPath()->SetPath(DLLTYPE_COMMON,
            (const char*)pBuffer->GetBuffer());
    }
    HX_RELEASE(pBuffer);

    // Find out if proxy is enabled in the license file(s)
    INT32 nProxyEnabled = 0;
    if (HXR_OK != proc->pc->registry->GetInt(REGISTRY_RTSPPROXY_ENABLED, &nProxyEnabled, proc))
    {
        nProxyEnabled = LICENSE_RTSPPROXY_ENABLED;
    }

    // Get the correct require plugins list, and make sure nobody
    // tampered with it by checking the checksum value
    char** ppRequiredPlugins = NULL;
    INT32 nChecksum = 0;
    INT32 nNumRequiredPlugins = 0;
        ppRequiredPlugins   = (char**)zm_pRequiredPlugins;
        nChecksum = zm_nReqPlugChecksum;
    nNumRequiredPlugins = GetNumRequiredPlugins(ppRequiredPlugins);

    // Create the plugin handler and load the plugins, paying
    // special attention to those plugins marked as "required"
    g_plugin_handler = new PluginHandler();
    g_plugin_handler->AddRef();
    result = g_plugin_handler->Init(proc->pc->server_context);
    if (FAILED(result))
    {
        ERRMSG(proc->pc->error_handler, "error loading plugins\n");
        terminate(1);
    }
    proc->pc->error_handler->Report(HXLOG_INFO, 0, 0, pMessage, 0);
    g_plugin_handler->SetRequiredPlugins((const char**)ppRequiredPlugins);
    if ((PluginHandler::NO_ERRORS != g_plugin_handler->Refresh()) ||
        (nChecksum != (nNumRequiredPlugins * -5)))
    {
        ERRMSG(proc->pc->error_handler, "error loading plugins\n");
        terminate(1);
    }

    proc->pc->plugin_handler = g_plugin_handler;

    // CSapManager needs plugin_handler, but when it gets created in
    // core_proc, plugin_handler's not been created.  So, if I pass
    // context right there, it fails.  So, init sap_mgr right here
    if (SUCCEEDED(proc->pc->sap_mgr->Init(proc->pc->server_context)))
    {
        proc->pc->sap_mgr->StartDirectory(SDP_MULTICAST_ADDR, SDP_MULTICAST_PORT);
    }
    if (proc->pc->multicast_mgr)
    {
        proc->pc->multicast_mgr->Init();
    }

    CoreSystemReadyCallback* cb = new CoreSystemReadyCallback;
#if !defined WIN32
#ifdef SHARED_FD_SUPPORT
    if (!g_bSharedDescriptors)
#endif /* SHARED_FD_SUPPORT */
    {
        cb->fdp = ((ControllerContainer *)proc->pc)->fdp_socket;
    }
#endif
    proc->pc->dispatchq->send(proc, cb, PROC_RM_CORE);

    misc_plugins = g_plugin_handler->m_general_handler->m_pPlugins;
    for (i = misc_plugins->Begin(); i != misc_plugins->End(); ++i)
    {
        PluginHandler::Plugin* m_plugin;
        m_plugin = (PluginHandler::Plugin*)(*i);
        if ((!(m_plugin->m_load_multiple)) || (m_plugin->m_generic))
        {
            CreateMiscProcCallback* cb = new CreateMiscProcCallback();
            cb->m_bNeedSocket = m_plugin->m_generic &&
                (m_plugin->m_load_multiple);
            cb->m_plugin = (PluginHandler::Plugin*) (*i);
            cb->m_pLBLAcceptor = 0;
            cb->func(proc); // Deletes itself
        }
    }

    if (proc->pc->multicast_mgr)
    {
	if (!proc->pc->multicast_mgr->SetupOK())
	{
	    HX_DELETE(proc->pc->multicast_mgr);
	}
	else
	{
	    //This needs to be done only after license plugin is loaded.
	    proc->pc->multicast_mgr->Init();
	}
    }

    if (g_bReportRestart)
    {
        g_bReportRestart = FALSE;
        ERRMSG(proc->pc->error_handler,
            "Server automatically restarted due to fatal error condition");
    }

    // Launch the memory reaper

    MemReaperProcessInitCallback* pReaper = new MemReaperProcessInitCallback;
    pReaper->m_proc = proc;
    *pMemreapProcInitMutex = 1;
    MakeProcess("memreap", pReaper, proc->pc->dispatchq, 0);
    while (*pMemreapProcInitMutex)
        microsleep(1);

    // Note that config.StreamerCount sets the initial number of streamers.
    // (It is possible that we'll start more later if needed.)
    // NumStreamers was the old undocumented config value name, use StreamerCount now.
    // Using ProcessorCount to set the number of streamers is the old bogus way
    // to configure this, no longer supported.
    INT32 numStrmr = 0;
    if ((HXR_OK != proc->pc->registry->GetInt("config.StreamerCount", &numStrmr, proc)) ||
       numStrmr == 0)
    {
        numStrmr = GetStreamerCount(proc);
    }
    *g_pNumStreamers = numStrmr;
    int j;
    for (j = 0; j < numStrmr; j++)
    {
        CreateStreamerCallback* cscb = new CreateStreamerCallback;
        cscb->func(proc);
    }

    // Init RTSP events manager.

    proc->pc->rtspstats_manager->Init((IUnknown*)proc->pc->server_context);
    proc->pc->sdpstats_manager->Init((IUnknown*)proc->pc->server_context);

    /*
     * Configure packet aggregation sizes
     */
    INT32 nAggVal = 0;
    char szCfgName[256];
    BOOL bAggSet = FALSE;
    for (j=0; j < 3; j++)
    {
        sprintf(szCfgName, "config.PacketAgg.%d.AggTo", j);
        if (HXR_OK == proc->pc->registry->GetInt(szCfgName, &nAggVal, proc))
        {
            g_pAggregateTo[j] = (UINT32)nAggVal;
            bAggSet = TRUE;
        }
        sprintf(szCfgName, "config.PacketAgg.%d.AggMax", j);
        if (HXR_OK == proc->pc->registry->GetInt(szCfgName, &nAggVal, proc))
        {
            g_pAggregateHighest[j] = (UINT32)nAggVal;
            bAggSet = TRUE;
        }
        sprintf(szCfgName, "config.PacketAgg.%d.MaxBitrate", j);
        if (HXR_OK == proc->pc->registry->GetInt(szCfgName, &nAggVal, proc))
        {
            g_pAggregateMaxBPS[j] = (UINT32)nAggVal;
            bAggSet = TRUE;
        }
    }
#ifdef _DEBUG
    if (bAggSet)
    {
        suprintf("Warning: Custom packet aggregation settings detected:\n");
    }
    for (j=0; bAggSet && j < 3; j++)
    {
        suprintf("\tconfig.PacketAgg.%d.AggTo:\t%d\n",
                j, g_pAggregateTo[j]);
        suprintf("\tconfig.PacketAgg.%d.AggMax:\t%d\n",
                j, g_pAggregateHighest[j]);
        suprintf("\tconfig.PacketAgg.%d.AggMaxBPS:\t%d\n",
                j, g_pAggregateMaxBPS[j]);
    }
#endif

    /*
     * Now that the refresh has happened we can let the core go..
     */
    *core_waiting_for_refresh = 0;

    new PortWatcher(proc);

    delete this;
}

INT16
CoreTransferCallback::GetNumRequiredPlugins(char** ppRequiredPlugins)
{
    INT16  nNumRequiredPlugins = 0;

    while(*ppRequiredPlugins++)
        nNumRequiredPlugins++;

    return nNumRequiredPlugins;
}

#if defined _LINUX || defined PTHREADS_SUPPORTED

#define SURE_THING_STACK_ALLOC (STACKSIZE * 2 - 1)

extern "C" void*
AllocateStack()
{
    void* pStack = 0;
    char* oldMapPtr = 0;
    char* mmapPtr;
    char* pAlignedStackTop;

    // children also call allocstack to get their signal stacks
    HXMutexLock(g_pAllocateStackMutex);

    // we mmap the stack memory so that we can guarantee it will be aligned
    // on STACKSIZE.  This way get_procnum_from_bos will be able to
    // OR the stack pointer with the STACKSIZE to fine the bottom of the stack.

    // Repeated calls tend to return contiguous memory, so let's try to get
    // the requests aligned on a STACKSIZE boundary, so we only need allocate
    // STACKSIZE bytes each time.  (Otherwise, we have to allocate
    // 2*STACKSIZE-1 bytes to guarantee at least one aligned stack.)

    size_t mapSize = STACKSIZE;
    while (1)
    {
        if ((mmapPtr = (char*) mmap(0, mapSize, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED)
        {
            perror("E: failed to allocate stack space");
            killme(1);
        }

        // Did the stack come out aligned?

        pAlignedStackTop = (char*)(((unsigned)mmapPtr | (STACKSIZE-1)) +
                                   1 - STACKSIZE);

        if (pAlignedStackTop == mmapPtr)
        {
            break;
        }
        else if (pAlignedStackTop + 2 * STACKSIZE <= mmapPtr + mapSize)
        {
            // we engulfed an aligned stack (happens if mmap grows upwards)
            pAlignedStackTop += STACKSIZE;
            break;
        }
        else
        {
            // we did the sure thing...

            if (mapSize == SURE_THING_STACK_ALLOC)
            {
                pAlignedStackTop += STACKSIZE;
#ifdef _DEBUG
                suprintf("AllocateStack(): Inefficient stack allocation at %p\n",
                         pAlignedStackTop);
#endif
                break;
            }

            // otherwise, release the memory for another try

            if (munmap(mmapPtr, mapSize))
            {
                perror("E: munmap failed in AllocateStack");
                killme(1);
            }

            //If this is the second time through, give up and do the sure thing
            // Set bMmapGrowsDown for the next AllocateStack() call

            if (mapSize != STACKSIZE)
            {
                *g_bMmapGrowsDown = (oldMapPtr != mmapPtr);
                mapSize = SURE_THING_STACK_ALLOC;
            }
            else
            {
                // How many more bytes do we need, to align the stack?
                // On linux, the allocations seem to grow downward.

                oldMapPtr = mmapPtr;
                if (*g_bMmapGrowsDown)
                {
                    mapSize = STACKSIZE +
                        ((int)mmapPtr - (int)pAlignedStackTop);
                }
                else
                {
                    mapSize = 2 * STACKSIZE -
                        ((int)mmapPtr - (int)pAlignedStackTop);
                }
            }
        }
    }
    pStack = pAlignedStackTop + STACKSIZE;  // return the stack bottom

    // the controller stack is the only one that's not aligned.
    // we use maxThreadStack to distinguish it.
    if (pStack > maxThreadStack)
    {
        maxThreadStack = (char*) pStack;
        HX_ASSERT(maxThreadStack < controllerBOS - STACKSIZE);
    }

    // end of critical section
    HXMutexUnlock(g_pAllocateStackMutex);

    // write in the magic word for use in determining stack usage
    memset(pAlignedStackTop, 0xff, STACKSIZE);

    // now protect the top of the stack against writing so we have a buffer
    // zone between stacks
#if !defined(_LINUX) // This is not compatable with NPTL
    mprotect(pAlignedStackTop, GUARD_SIZE, PROT_NONE);
#endif //!defined(_LINUX)

    return pStack;
}

#elif defined _AIX

void*
AllocateStack()
{
    return (char*)malloc(STACKSIZE) + STACKSIZE;
}

#endif // _LINUX || PTHREADS_SUPPORTED

#ifdef PTHREADS_SUPPORTED

// this func was made specifically to address the problem with creating a
// resolver thread.
extern "C" void
RecordStackBottom(Process* proc)
{
    // record our stack bottom in the array for determining stack usage
    // note: address of any stack var is ok for get_base_of_stack
    g_threadStacks[proc->procnum()].pStack = (char*)get_base_of_stack((size_t)&proc) - STACKSIZE;
}

void*
PthreadEntryPoint(thread_args* ta)
{
    SimpleCallback*     cb = ta->cb;
    DispatchQueue*      dispatch_queue = ta->dispatch_queue;
    Process*            proc;

    // set up this thread'/process's signal stack
    stack_t ss;
    memset(&ss, 0, sizeof(stack_t));
    ss.ss_sp = AllocateStack();

    *((void**)ss.ss_sp - 1) = ta;
    ss.ss_size = STACKSIZE - sizeof(thread_args*) - GUARD_SIZE;

    ss.ss_sp = (char*)ss.ss_sp - STACKSIZE + GUARD_SIZE;
    sigaltstack(&ss, NULL);

    ta->process_number = Process::GetNewProcessNumber();
    g_nTLSProcnum = ta->process_number;

    proc = new Process;
    proc->AssignProcessNumber(ta->process_number);
    ta->proc = proc;

    RecordStackBottom(proc);

#ifdef _LINUX
    suprintf("Starting TID %lu/%d, procnum %d (%s)\n",
           pthread_self(), gettid(), proc->procnum(), ta->processname);
#else
    suprintf("Starting TID %lu, procnum %d (%s)\n",
           pthread_self(), proc->procnum(), ta->processname);
#endif

#if defined PTHREADS_SUPPORTED && defined _LINUX
    AddToThreadList(pthread_self());
#endif

#ifdef _LINUX
    AddToProcessList(gettid());
#else
    AddToProcessList(pthread_self());
#endif

#ifdef ENABLE_PROFILING
    ProfilingSetup();
#endif

    cb->func(proc);

    // XXXSMP - process exited!!
    suprintf("Thread %d exited\n", ta->process_number);
    fflush(stdout);
    return 0;
}

int*
MakeThread(const char* processname,
           SimpleCallback* cb,
           DispatchQueue* dispatch_queue,
           int flags)
{
    // this variable better stay the same!!!
    static int s[2] = { -1, -1 };

    thread_args* ta = new thread_args;
    ta->cb = cb;
    ta->dispatch_queue = dispatch_queue;
    ta->processname = (char*)processname;
    ta->proc = 0;
    ta->process_number = 0;

    pthread_t tid = 0;

    pthread_attr_t* attr = new pthread_attr_t;

    pthread_attr_init(attr);
    void** pStack = (void**)AllocateStack();
    *(pStack - 1) = ta;
    size_t stacksize = STACKSIZE - GUARD_SIZE - sizeof(thread_args*);;
    pStack = (void**)((char*)pStack - STACKSIZE + GUARD_SIZE);

#ifdef __USE_XOPEN2K
    // used on Linux w/ NPTL
    pthread_attr_setstack(attr, pStack, stacksize);
#else
    // these are depreciated in Linux (and it doesn't work right either w/ NPTL)
    pthread_attr_setstackaddr(attr, pStack);
    pthread_attr_setstacksize(attr,  stacksize);
#endif

    pthread_attr_setscope(attr, PTHREAD_SCOPE_SYSTEM);

    if (pthread_create(&tid, attr, (void*(*)(void*))PthreadEntryPoint, ta) != 0)
    {
        char* pSysError = strerror(errno);
        suprintf("Creation of %s thread failed: %s\n", processname, pSysError);
        exit(1);
    }

    return s;
}

#endif // PTHREADS_SUPPORTED

#if !defined _WIN32

int*
MakeProcess(const char* processname, SimpleCallback* cb,
    DispatchQueue* dispatch_queue, int flags, void* linuxstuff)
{

#if defined PTHREADS_SUPPORTED
    if (g_bUnixThreads && !(flags & CP_FORK_PROCESS))
        return MakeThread(processname, cb, dispatch_queue, flags);
#endif // defined PTHREADS_SUPPORTED

    Process*            proc = 0;
    int                 pid = 0;
    int                 process_number = 0;
    static int          s[2] = { -1, -1 };
    struct passwd*      pPwEnt = getpwuid(geteuid());
    const char*         pUsername = pPwEnt ? pPwEnt->pw_name : 0;

        process_number = Process::GetNewProcessNumber();
        if (flags & CP_CREATE_SOCKET)
        {
            socketpair(AF_UNIX, SOCK_STREAM, 0, s);
        }

        /* New Random seed! */
        srand((unsigned)time(NULL) + process_number);
        pid = fork();

    switch (pid)
    {
        case 0:     /* Child */

#if defined _LINUX || defined _SOLARIS
            if (!g_bNoCrashAvoidance)
            {
                // set up this thread'/process's signal stack
                stack_t ss;
                memset(&ss, 0, sizeof(stack_t));
                ss.ss_sp = AllocateStack();

                ss.ss_sp = (char*)ss.ss_sp - STACKSIZE;
                sigaltstack(&ss, NULL);
            }
#endif // defined _LINUX || defined _SOLARIS

#if defined DEBUG && (defined _LINUX || defined _FREEBSD || defined _OPENBSD || defined _NETBSD)
            char buffer[2048];
            sprintf(buffer, "%s (%s)", ServerVersion::ExecutableName(), processname);
            procstr((char *)buffer);
#endif
	    g_nTLSProcnum = process_number;

            proc = new Process;
            proc->AssignProcessNumber(process_number);
            if (g_bPrintProcessPIDs)
            {
                suprintf ("Starting PID %d, procnum %d (%s)\n", getpid(),
                    process_number, processname);
            }

#ifdef ENABLE_PROFILING
            ProfilingSetup();
#endif

            DPRINTF(D_INFO, ("Process #%d PID %d Started (%s)\n",
                proc->procnum(), proc->procid(proc->procnum()), processname));

            cb->func(proc);
            // XXXSMP - process exited!!
            break;

        case -1:    /* Error */
            /*
             * No process to send error message, so just print it out
             */
            switch(errno)
            {
                case EAGAIN:
                    suprintf("<%s> has too many forked processes!\n",
                        pUsername ? pUsername : "User");
                    break;
                case ENOMEM:
                    suprintf("fork() failed due to INSUFFICIENT SWAP space!\n");
                    break;
                default:
                    suprintf("fork() failed!\n");
                    break;
            }
            fflush(0);
            terminate(1);

      default:    /* Parent */

            AddToProcessList(pid);
            return s;
    }

    return 0;  // Shut up GCC
}

#else

DWORD WINAPI WIN32_ThreadStart(void* p)
{
    thread_args* ta = (thread_args*)p;
    SimpleCallback*     cb;
    DispatchQueue*      dispatch_queue;
    Process*            proc;
    int                 process_number;

    cb = ta->cb;
    dispatch_queue = ta->dispatch_queue;

    process_number = Process::GetNewProcessNumber();
    g_nTLSProcnum = process_number;

    proc = new Process;
    proc->AssignProcessNumber(process_number);
    suprintf("Starting TID %d, procnum %d (%s)\n", GetCurrentThreadId(), proc->procnum(), ta->processname);

    delete ta;

    cb->func(proc);
    // XXXSMP - process exited!!
    suprintf("Thread exited\n");
    fflush(stdout);
    return 0;
}

int*
MakeProcess(const char* processname, SimpleCallback* cb,
    DispatchQueue* dispatch_queue, int flags)
{
    HANDLE              ThreadHandle;
    thread_args*        ta;
    static int          s[2] = { -1, -1 };

    ta = new thread_args;
    ta->cb = cb;
    ta->dispatch_queue = dispatch_queue;
    ta->processname = (char*)processname;

    DWORD dwId;

    ThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WIN32_ThreadStart,
            (void*)ta, 0, &dwId);

    while (!Process::set_handle_of_thread(ThreadHandle, dwId))Sleep(0);

    if ((u_long32)ThreadHandle < 0)
    {
        PANIC(("Unable to start server->mainloop %d", GetLastError()));
        return 0;
    }

    return s;
}

#endif // ! defined _WIN32

void
CreateStreamerCallback::func(Process* proc)
{
    int *s;
    int p[2] = { -1, PROC_RM_CORE };

    DPRINTF(D_INFO, ("Streamer Process starting up\n"));
    StreamerProcessInitCallback* cb = new StreamerProcessInitCallback;
    cb->m_proc = proc; // Main's Process
    *pStreamerProcInitMutex = 1;

    s = MakeProcess("streamer", cb, proc->pc->dispatchq, CP_CREATE_SOCKET);
    while (*pStreamerProcInitMutex)
        microsleep(1);

    CoreFDPassSocketCallback* cfdpcb = new CoreFDPassSocketCallback;
    cfdpcb->m_pAllowSendMutex = g_pProcessStartInProgress;
    cfdpcb->m_bNeedEnableStreamer = TRUE;
    cfdpcb->streamer_num = proc->numprocs() - 1;
    cfdpcb->acceptor = 0;

#ifndef _WIN32
#ifdef SHARED_FD_SUPPORT
    if (!g_bSharedDescriptors)
#endif /* SHARED_FD_SUPPORT */
    {
        p[0] = cfdpcb->streamer_num;
        cfdpcb->fdp = new FDPassSocket(s, p, proc);
        ((ControllerContainer *)proc->pc)->fdp_socket->
            Send(cfdpcb->fdp->GetSocket0(), PROC_RM_CORE);
    }
#endif /* !_WIN32 */

#ifdef _WIN32
    *g_pProcessStartInProgress = TRUE;
#endif /* _WIN32 */

    HXAtomicIncUINT32(g_pStreamerCount);

    proc->pc->dispatchq->send(proc, cfdpcb, PROC_RM_CORE);

    delete this;
}

void
CreateMiscProcCallback::func(Process* proc)
{
    int *s = 0;
    int p[2] = { -1, PROC_RM_CORE };
    DPRINTF(D_INFO, ("Misc Process starting up\n"));

    MiscProcessInitCallback* mcb = new MiscProcessInitCallback();
    mcb->m_proc = proc;
    mcb->m_plugin = m_plugin;
    mcb->m_pAcceptor = m_pLBLAcceptor;
    *pMiscProcInitMutex = 1;

    s = MakeProcess("rmplug", mcb, proc->pc->dispatchq,
        m_bNeedSocket ? CP_CREATE_SOCKET : 0);
    while (*pMiscProcInitMutex)
        microsleep(1);

    if (m_bNeedSocket)
    {
        CoreFDPassSocketCallback* cb = new CoreFDPassSocketCallback;
        if (m_pLBLAcceptor)
        {
            cb->m_pAllowSendMutex = &m_pLBLAcceptor->m_pCDispatch->
                                                            m_bStartInProgress;
            cb->m_pLBLConnDispatch = m_pLBLAcceptor->m_pCDispatch;
        }

#ifndef _WIN32
#ifdef SHARED_FD_SUPPORT
        if (!g_bSharedDescriptors)
#endif /* SHARED_FD_SUPPORT */
        {
            p[0] = proc->numprocs() - 1;
            cb->fdp  = new FDPassSocket(s, p, proc);
            cb->streamer_num = p[0];
            ((ControllerContainer *)proc->pc)->fdp_socket->
                Send(cb->fdp->GetSocket0(), PROC_RM_CORE);
        }
#endif /* !_WIN32 */

        cb->acceptor = m_pLBLAcceptor;

#ifdef _WIN32
        *g_pProcessStartInProgress = TRUE;
#endif /* _WIN32 */

        proc->pc->dispatchq->send(proc, cb, PROC_RM_CORE);
    }

    delete this;
}

void
CreateResolverCallback::func(Process* proc)
{
    char szMsg[80];
    sprintf(szMsg, "%s Resolver starting up\n", ServerVersion::ProductName());
    DPRINTF(D_INFO, (szMsg));
    ResolverProcessInitCallback* cb = new ResolverProcessInitCallback;
    cb->m_proc = proc; // Main's Process
    cb->calling_procnum = calling_procnum;
    *pResolverProcInitMutex = 1;

    MakeProcess("resolver", cb, proc->pc->dispatchq, 0);
    while (*pResolverProcInitMutex)
        microsleep(1);

    delete this;
}

WriteConfigCallback::WriteConfigCallback()
    : m_pKeyName(0)
{
}

WriteConfigCallback::WriteConfigCallback(const char * pKeyName)
    : m_pKeyName(0)
{
    if (pKeyName && *pKeyName)
    {
        m_pKeyName = new char[strlen(pKeyName) + 1];
        strcpy(m_pKeyName, pKeyName);
    }
}

WriteConfigCallback::~WriteConfigCallback()
{
    HX_VECTOR_DELETE(m_pKeyName);
}

void
WriteConfigCallback::func(Process* proc)
{
    DPRINTF(D_INFO, (" Core Process writing Server Config\n"));

    HX_ASSERT(proc->pc->process_type == PTCore);
    m_pProc = proc;

    proc->pc->engine->ischedule.enter(Timeval(0.0), this);
}
 
 
STDMETHODIMP
WriteConfigCallback::Func()
{
    m_pProc->pc->config_file->WriteKey(
                   (m_pKeyName && *m_pKeyName) ? m_pKeyName : "config");
      
    return HXR_OK;
}

/*
 * Windows/Unix fault handlers.
 */

/*
 * If we are going to return 0, then we must
 * fill in the pJB
 */
ServerEngine*
get_my_engine()
{
    int procnum;
    if (!(*g_ppLastEngine))
    {
        return 0;
    }
    /*
     * WARNING!  The proc we get here is not our proc.
     *     the only thing that is ours is the procnum.
     */
    Process* proc = (Process*)(*g_ppLastEngine)->get_proc();

    // XXXtbradley Process::get_procnum should work anywhere but I
    // haven't figured out how to get it to work from the sigalt stack
    // on LINUX so we need this hack.
#if defined _LINUX  && !defined PTHREADS_SUPPORTED
    procnum = Process::translate_procnum(getpid());
#else
    procnum = Process::get_procnum();
#endif // _LINUX

    if (!proc || procnum < 0)
    {
        suprintf("No proc or procnum for pid %d\n", getpid());
        return 0;
    }

    /*
     * Finally! We have our engine!
     */
    ServerEngine* pEngine = proc->get_engine(procnum);
    if (!pEngine)
    {
        suprintf("No Server Engine for procnum %d, pid%d\n",
            procnum, getpid());
        return 0;
    }

    return pEngine;
}

#if defined _LINUX || defined PTHREADS_SUPPORTED

//
// returns the base of the main thread stack. 
//
size_t
get_base_of_stack(size_t sp)
{
    //
    // we assume that the controller stack is allocated at the top of
    // the process's address space and so all thread stacks will be
    // below it.
    //
    if (sp > (size_t)maxThreadStack)
        return (size_t)controllerBOS;

    //
    // otherwise bump the pointer up to the bottom of the stack (stacks grow down)
    //
    size_t bos =(sp | (STACKSIZE - 1)) + 1;

    return bos - 1;
}

#endif // _LINUX || PTHREADS_SUPPORTED

#ifdef _UNIX
BOOL IAmAliveChecker();
BOOL IAmMonitor();
#define RSSLOG(x) if (!IAmAliveChecker() && !IAmMonitor()) { RSSManager::CrashOutput(x); }
#else
#define RSSLOG(x) { RSSManager::CrashOutput(x); }
#endif

#define CA_PRINT(x) if (g_bCrashAvoidancePrint) { printf x ;  fflush(stdout); }
#define SDEM_PRINT(x) if (g_bShowDebugErrorMessages) { printf x ; fflush(stdout); }

#ifdef _LINUX
extern "C" void CrashOutputWrapper(const char *pString)
{
    if (g_bCrashAvoidancePrint)
    {
        printf("%s", pString);
        fflush(stdout);
    }

    RSSLOG(pString);
}
#endif

#if defined _LINUX || defined _SOLARIS || defined _AIX
void
dump_info(BOOL bIsCrash, int ulCrashState, const char* pTraceStr, int code, siginfo_t* pSI, ucontext_t* pSC)
#else
void
dump_info(BOOL bIsCrash, int ulCrashState, const char* pTraceStr, int code)
#endif // _LINUX
{
    char buffer[8192];
    char *ptr = &(buffer[0]);
    ptr += sprintf(ptr,
        "\n-------------------------------------------------------------------------------\n");
    ptr += sprintf(ptr, "*** %s %s Report\n", ServerVersion::ProductName(),
           bIsCrash ? "Crash" : "Fault");

    // TIME
    struct tm tm;
    HXTime now;
    gettimeofday(&now, NULL);
    hx_localtime_r((const time_t *)&now.tv_sec, &tm);
    ptr += strftime(ptr, (sizeof buffer)-strlen(buffer), "When: %d-%b-%y %H:%M:%S\n", &tm);

    ptr += sprintf(ptr, "Environment: %s, %s, %s\n", ServerVersion::Platform(),
           ServerVersion::BuildName(), ServerVersion::VersionString());

#ifdef _WIN32
    ptr += sprintf(ptr, "TID %lu\n", GetCurrentThreadId());
    ptr += sprintf(ptr, "Caught Exception %x\n", code);
#else
#ifdef PTHREADS_SUPPORTED
    ptr += sprintf(ptr, "TID %lu\n", pthread_self());
#else
    ptr += sprintf(ptr, "PID %d\n", getpid());
#endif
    const char* szSigName = strsignal(code);
    if (szSigName)
    {
        ptr += sprintf(ptr, "Caught Signal %d (%s)\n", code, szSigName);
    }
    else
    {
        ptr += sprintf(ptr, "Caught Signal %d\n", code);
    }
#endif
    ptr += sprintf(ptr, "Fault State: %d\n\n", ulCrashState);
    ptr += sprintf(ptr, "Stack Trace:\n");

    CA_PRINT(("%s", buffer));
    RSSLOG(buffer);
    ptr = &(buffer[0]);

    if (pTraceStr)
    {
        CA_PRINT(("%s\n", pTraceStr));
        RSSLOG(pTraceStr);
        RSSLOG("\n");
    }
    else
    {
#if defined _WIN32
        /*
         * Sorry I have to do this here but I have to get
         * last known eip in a wierd way.
         */
        UINT32 ulAt = g_last_known_eip;
        UINT32* pFrame;
        UINT32* pPrevFrame;
        pFrame = (unsigned long *)g_last_known_ebp;
        while (1)
        {
            sprintf(buffer, "0x%x\n", ulAt);
            CA_PRINT(("%s", buffer));
            RSSLOG(buffer);
            ulAt = pFrame[1];
            pPrevFrame = pFrame;
            pFrame = (UINT32*)pFrame[0];
            if ((UINT32)pFrame & 3)
                break;
            if (pFrame <= pPrevFrame)
                break;
            if (IsBadWritePtr(pFrame, sizeof(void*)*2))
                break;
            // output each time to avoid buffer overflow
        };
        ptr = &(buffer[0]);

        CA_PRINT(("\n\n"));
        RSSLOG("\n\n");

#elif defined _LINUX || defined _SOLARIS || defined _AIX
        char* pTrace = get_trace_from_context(pSC);
        CA_PRINT(("\n%s\n\n", pTrace));
        RSSLOG("\n");
        RSSLOG(pTrace);
        RSSLOG("\n\n");

#else
        char* pTrace = get_trace();
        CA_PRINT(("\n%s\n\n", pTrace));
        RSSLOG("\n");
        RSSLOG(pTrace);
        RSSLOG("\n\n");

#endif
    }
    ptr += sprintf(ptr, "-------------------------------------------------------------------------------\n");

    CA_PRINT(("%s", buffer));
    RSSLOG(buffer);
}


// Attempt to use a lock to prevent multiple CAs that occur in different
// processes at almost the same time from garbling their printf output
// together.  If we don't get the lock we don't want to risk deadlock
// so we return after trying to get the lock a number of times, which
// may result in somewhat garbled output, but is otherwise harmless.
inline void
GetCrashPrintLock()
{
    if (g_pCrashPrintLock)
    {
        int i=10;
        while (--i)
        {
            if (HXMutexTryLock(g_pCrashPrintLock))
                break;
            microsleep(10000);
        }
    }
}

inline void
ReleaseCrashPrintLock()
{
    if (g_pCrashPrintLock)
        HXMutexUnlock(g_pCrashPrintLock);
}

// restart/kill the server if too many CAs have occurred
void
DieIfTooManyCAs()
{
    if (g_pCrashAvoidWindowTime && g_pNow->tv_sec > g_pCrashAvoidWindowTime->tv_sec + (60*60*4))
    {
        // CA window time reached, reset window and window counter
        *g_pCrashAvoidWindowTime = *g_pNow;
        *g_pNumCrashAvoidsInWindow = 0;
        return;
    }

    if (g_pNumCrashAvoidsInWindow && g_ulMaxCrashAvoids > 0 &&
        *g_pNumCrashAvoidsInWindow >= g_ulMaxCrashAvoids)
    {
#ifdef _LINUX
        // for some reason on linux we end up back here and in an infinite loop
        // so we just note if we've been through here already and exit if so.
        static BOOL g_bAlreadyTriedThis = FALSE;
        if (!g_bAlreadyTriedThis)
            g_bAlreadyTriedThis = TRUE;
        else
            _exit(0);
#endif // _LINUX

        if (g_bNoAutoRestart)
        {
            printf("Server Shutdown: Too many faults (%lu, since startup: %lu), exiting "
                   "(restart disabled)\n", *g_pNumCrashAvoidsInWindow, *g_pNumCrashAvoids);
            fflush(0);
            terminate(1);
        }
        else
        {
            printf("Server Restart: Too many faults (%lu, since startup: %lu), restarting\n",
                   *g_pNumCrashAvoidsInWindow, *g_pNumCrashAvoids);
            fflush(0);

            /*
             * Tear down the current server and allow monitor to
             * restart it.
             */
            RestartServer();
            return;
        }
    }
}

#ifdef _WIN32

//#include "hxtrace.h"
#define HLXSERVER_PURECALL_EXCEPTION 0xE0000000
int g_got_dummy_call = 1;

void
dummy_call(DWORD code)
{
    g_got_dummy_call = 1;

    DieIfTooManyCAs();

    if (g_eTLSCrashState == NO_FAULT)
    {
        g_eTLSCrashState = IN_FAULT_HANDLER;

        if (g_pNumCrashAvoids)
        {
            HXAtomicIncUINT32(g_pNumCrashAvoids);
        }
        if (g_pNumCrashAvoidsInWindow)
        {
            HXAtomicIncUINT32(g_pNumCrashAvoidsInWindow);
        }

        GetCrashPrintLock();
        dump_info(FALSE, g_eTLSCrashState, 0, code);
        ReleaseCrashPrintLock();

        g_eTLSCrashState = NO_FAULT;
        if (bRestartOnFault)
        {
            RestartOrKillServer();
        }

        ServerEngine* pEngine = get_my_engine();

        pEngine->CrashRecover();
        ASSERT(0);
    }
    else if (g_eTLSCrashState == IN_FAULT_HANDLER)
    {
        g_eTLSCrashState = IN_FAULT_DOUBLE_FAULT_HANDLER;

        dump_info(FALSE, g_eTLSCrashState, "Double Fault, No Trace", code);
        ReleaseCrashPrintLock(); // was locked in first crash handler

        g_eTLSCrashState = NO_FAULT;
        if (bRestartOnFault)
        {
            RestartOrKillServer();
        }

        ServerEngine* pEngine = get_my_engine();

        pEngine->CrashRecover();
        ASSERT(0);
    }
    else if (g_eTLSCrashState == IN_FAULT_DOUBLE_FAULT_HANDLER)
    {
        g_eTLSCrashState = IN_FAULT_DOUBLE_FAULT_HANDLER;

        if (g_bCrashAvoidancePrint)
        {
            printf ("Triple Fault:  Directly JumpStarting Engine\n");
        }
        RSSManager::CrashOutput("Triple Fault: Directly JumpStarting Engine\n");

        ReleaseCrashPrintLock(); // was locked in first crash handler

        g_eTLSCrashState = NO_FAULT;
        if (bRestartOnFault)
        {
            RestartOrKillServer();
        }

        ServerEngine* pEngine = get_my_engine();

        pEngine->mainloop(TRUE);
        ASSERT(0);
    }
}

LONG32
server_fault(_EXCEPTION_POINTERS *pExceptionInfo)
{
    CONTEXT* pContext = pExceptionInfo->ContextRecord;
    DWORD code = pExceptionInfo->ExceptionRecord->ExceptionCode;
    g_last_known_eip = pContext->Eip;
    g_last_known_ebp = pContext->Ebp;

    BOOL b_dummy_ok = 1;
    if (!g_got_dummy_call)
    {
        b_dummy_ok = 0;
        SetUnhandledExceptionFilter(NULL);
    }
    g_got_dummy_call = 0;

    BOOL bSafeToLJFromHere = 0;
    OSVERSIONINFO winver;
    winver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&winver))
    {
        if (winver.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            bSafeToLJFromHere = 1;
        }
    }
    /*
     * IMPORTANT- only ever ever do the following
     * under NT.  98 GOES CRAZY if you try this.
     */
    if (bSafeToLJFromHere && b_dummy_ok)
    {
        dummy_call(code);
    }
    else
    {
        /*
         * This works under some situations.
         */
        pContext->Eip = (unsigned long)dummy_call;
    }
    return EXCEPTION_CONTINUE_EXECUTION;
}

void
PurecallHandler(void)
{
    RaiseException(HLXSERVER_PURECALL_EXCEPTION, 0, 0, NULL);
}

#endif /* _WIN32 */
#ifdef _UNIX

/*  Signals
 *
 *  HUP - reconfig
 *  USR1 - kill select
 *  USR2 - restart
 */
#define RECONFIG_SIGNAL SIGHUP
#define RESTART_SIGNAL  SIGUSR2

/* debug signals */
#define RESETLEAKS_SIGNAL SIGVTALRM
#define CHECKLEAKS_SIGNAL SIGWINCH
#define CHECKGUARDS_SIGNAL SIGIO
#define RESETSIZES_SIGNAL SIGURG
#define CHECKSIZES_SIGNAL SIGTTOU
#define SUSPENDLEAKS_SIGNAL SIGTTOU
#define STOPSIZES_SIGNAL SIGXCPU
#define SOCKETTIMES_SIGNAL SIGTTIN

#ifdef MEMCACHE_DUMP
#define MEMCACHE_DUMP_SIGNAL SIGILL
#endif

/*
 * Used for clean restarts of the server (RESTART_SIGNAL, Admin Restart)
 * Safe to call from ANY process.
 */
void
perform_restart()
{
    if (IAmController())
    {
        /* Signal the monitor that this is a clean restart (C) */
        write(MonitorControllerSocket[1], "C", 1);
        /* Tear down the current server and allow monitor to restart it */
        *g_pbRestarting = TRUE;
	_exit(RESTART_SIGNAL);

        return;
    }

#ifdef PTHREADS_SUPPORTED
    if (g_bUnixThreads && !IAmMonitor() && !IAmAliveChecker())
    {
        /* Pass the restart signal up to the controller */
        pthread_kill(g_ControllerTid, RESTART_SIGNAL);
        return;
    }
#endif // PTHREADS_SUPPORTED

    /* Pass the restart signal up to the controller */
    kill(g_ControllerPid, RESTART_SIGNAL);
}

void
handle_child(int code, siginfo_t* pSigInfo, ucontext_t* pSC)
{
    int status;
    pid_t childpid;

    while ((childpid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        DPRINTF(D_XFER|D_INFO, ("SIGCHLD received by %u, pid %d status %s\n",
                getpid(), (int)childpid,
                (WIFEXITED(status) ? "exited" :
                (WIFSIGNALED(status) ? "signalled" : "unknown"))));

        if (g_bShowDebugErrorMessages)
        {
#ifdef PTHREADS_SUPPORTED
	    pthread_t pid = pthread_self();
#else
	    int pid = getpid();
#endif
            if (WIFEXITED(status))
            {
                printf("%lu: SIGCHLD received, pid %d exited(%d): "
                       "child exit with status %d\n", pid,
                       (int)childpid, status, WEXITSTATUS(status));
            }
            else if (WIFSIGNALED(status))
            {
                printf("%lu: SIGCHLD received, pid %d status signaled(%d): "
                       "child exited due to unhandled signal %d)\n",
		       pid, (int)childpid, status, WTERMSIG(status));
            }
            else
            {
                printf("%lu: SIGCHLD received, pid %d status unknown(%d)\n",
                    pid, (int)childpid, status);
            }
            fflush(0);
        }

        if (g_pbHandleCHLD && *g_pbHandleCHLD == FALSE)
        {
            return;
        }

	if (WIFSIGNALED(status) && pSigInfo->si_status == RESTART_SIGNAL)
	{
	    g_bNoAutoRestart = FALSE;
	}

        for (int i = 0; i < MAX_THREADS; i++)
        {
            if (g_GlobalProcessList[i] == 0)
            {
                return;
            }

            if (g_GlobalProcessList[i] == childpid)
            {
                if (g_pbHandleCHLD && *g_pbHandleCHLD == FALSE)
                {
                    return;
                }
                if (g_bNoAutoRestart)
                {
                    printf("Server Shutdown:  process %d exited, "
                           "auto-restart is disabled.\n", childpid);
                    /*
                     * Tear down the current server and shutdown
                     */
                    struct rlimit r = {0, 0};
                    setrlimit(RLIMIT_CORE, &r);

                    killme(0);
                }
                else
                {
                    if (g_bShowDebugErrorMessages)
                    {
                        printf ("SIGCHLD received, restarting\n");
                        fflush(0);
                    }

                    /*
                     * Tear down the current server and allow monitor to
                     * restart it.
                     */
                    *g_pbRestarting = TRUE;

                    struct rlimit r = {0, 0};
                    setrlimit(RLIMIT_CORE, &r);

                    killme(0);
                    return;
                }
            }
        }
    }
    return;
}

BOOL
ShutdownServer(BOOL bManually)
{
    if (bManually)
    {
        g_bNoAutoRestart = 1;
        gracefully_killme(0);
    }
    else
    {
        terminate(1);
    }

    return TRUE;
}

BOOL
RestartServer(BOOL bManually)
{
    if (bManually)
    {
        gracefully_killme(SELF_DEFINE_RESTART_SIGNAL);
    }
    else
    {
        perform_restart();
    }

    return TRUE;
}

void
dump_trace(int code)
{
    if (IAmController())
    {

#if defined PTHREADS_SUPPORTED && defined _SOLARIS

        // pstack on solaris will print the stacks of all the lwps in the
        // server
        char str[128];
        snprintf(str, 127, "/usr/proc/bin/pstack %d\n", getpid());
        str[127] = 0;
        printf("\n------------------------ %d -------------------------\n");
        fflush(stdout);
        system(str);

#elif defined PTHREADS_SUPPORTED && defined _LINUX

        char *pTrace = get_trace();

        CA_PRINT(("\n------------------------ %lu/%d -------------------------\n", pthread_self(), gettid()));
        CA_PRINT(("%s", pTrace));

        // send signals to other threads created by controller for printing their stacks.
        for (int i = 0; i < MAX_THREADS; i++)
        {
            if (g_GlobalThreadList[i] != 0)
            {
                pthread_kill(g_GlobalThreadList[i], SIGXFSZ);
                microsleep(100000); //pause 1/10th second for output to flush
            }
        }

#elif defined PTHREADS_SUPPORTED 

        // XXXtbradley we need to handle this case on non-solaris platforms
#warning dump_trace not implemented on this platform with PTHREADS_SUPPORTED
#else

        for (int i = 0; i < MAX_THREADS; i++)
            if (g_GlobalProcessList[i] != 0 &&
                g_GlobalProcessList[i] != getpid() &&
                g_GlobalProcessList[i] != g_MonitorPID &&
                g_GlobalProcessList[i] != g_AliveCheckerPID)
            {
                kill(g_GlobalProcessList[i], SIGXFSZ);
                microsleep(100000); //pause 1/10th second for output to flush
            }

#endif // defined PTHREADS_SUPPORTED && defined _SOLARIS


    }
    else
    {
        char buffer[zMaxOpenFDs];
        char *ptr = &(buffer[0]);

#if defined PTHREADS_SUPPORTED && defined _LINUX
        char *pTrace = get_trace();

        CA_PRINT(("\n------------------------ %lu/%d -------------------------\n", pthread_self(), gettid()));
        CA_PRINT(("%s", pTrace));


#elif defined PTHREADS_SUPPORTED && !defined _SOLARIS

        // XXXtbradley we need to handle this case on non-solaris platforms
#warning dump_trace not implemented on this platform with PTHREADS_SUPPORTED

#elif !defined _SOLARIS

        //Do this in a single printf to improve our chances of getting it
        //all printed together and not be garbled by other processes' output:
        ptr += sprintf(ptr, "\n------------------------ %d -------------------------\n"
               "Heartbeat Stack Trace:\n%s\n", getpid(), get_trace());

        CA_PRINT(("%s", buffer));
        RSSLOG(buffer);
#else

        // XXX vshetty 10/22/2003
        //
        // Solaris implementation needs a future modification to support printing
        // heartbeat stacks to RSS Log. One possible solution is to use the
        // get_trace method to get the stack instead of pstack.
        char str[128];
        printf("\n------------------------ %d -------------------------\n"
               "Heartbeat Stack Trace:\n", getpid());
        sprintf(str, "/usr/proc/bin/pstack %d \n", getpid());

        /* Temporarily return to original UID/GID so pstack can access our proc */
        uid_t uid = geteuid();
        uid_t gid = getegid();
        setuid(getuid());
        setgid(getgid());

        system(str);

        /* Return to UID/GID specified in the config file */
        setuid(uid);
        setgid(gid);
#endif
    }
}

void gracefully_killme(int code)
{
    if (code == SELF_DEFINE_RESTART_SIGNAL && *g_bEnableClientConnCleanup == FALSE)
    {
        perform_restart();
        return;
    }
    else if (*g_bEnableClientConnCleanup == FALSE)
    {
	shared_ready = 0;
	*g_pbHandleCHLD = FALSE;

#ifdef PAULM_ALLOCDUMP
	if (IAmController())
	{
	    printf("Flushing alloc...\n");
	    flush_alloc();
	    printf("Done.\n");
	}
#endif

	fflush(0);
	_exit(0);
    }

    UINT32 retval = HXAtomicIncRetUINT32(g_pSignalCount);
    // only one process will do the job
    if (retval > 1)
    {
        return;
    }

    Process* proc = NULL;
    int processnum = 0;
    int total_processes = 0;

    total_processes = Process::numprocs();

    processnum = Process::GetNewProcessNumber();

    proc = new Process;
    proc->AssignProcessNumber(processnum);

    StopAliveChecker();
    // force to kick-off the mainloop in each process
    // terminated = TRUE;
    for (int i = 0; i < total_processes; i++)
    {
        if (i != 1 && i != PROC_RM_CONTROLLER)
	{
	    // make sure that the receving thread has a dispatchq unlike the
	    // resolver thread which doesn't
	    if (g_dq->has_dispatchq(i))
	    {
	    ShutdownCallback* cb = new ShutdownCallback;
	    cb->m_bRestart = (code == SELF_DEFINE_RESTART_SIGNAL);
            g_dq->send(proc, cb, i);
	}
    }
}
}

void
killme(int code)
{
    if (IAmController() && (*g_pbRestarting == FALSE))
    {
        /* Signal the monitor that a clean shutdown occured */
        write(MonitorControllerSocket[1], "S", 1);
    }

    shared_ready = 0;
    *g_pbHandleCHLD = FALSE;

#ifdef PAULM_ALLOCDUMP
    if (IAmController())
    {
        printf("Flushing alloc...\n");
        flush_alloc();
        printf("Done.\n");
    }
#endif

    killpg(0, SIGKILL);

    fflush(0);
    _exit(0);
}

void
crash_print(int code)
{
#ifdef _DEBUG
    switch(code)
    {
#ifdef SIGXCPU
        case SIGXCPU:
            printf("SIGXCPU Caught!\n");
            fflush(stdout);
            return;
#endif

        default:
            printf("signal %d Caught!\n", code);
            fflush(stdout);
            return;
    }
#endif //_DEBUG
}

#if defined _LINUX || defined _SOLARIS || defined _AIX
void
server_fault(int code, siginfo_t* pSiginfo, ucontext_t* pSC)
#else
void
server_fault(int code)
#endif // _LINUX
{
#if defined _LINUX || defined _SOLARIS || defined _SUN
    /*
     * remove the pending synchronous signal from the sigmask
     * so that it can be delivered again. if this is not done
     * then in the case of double/triple faults the default action
     * will be taken and the thread will die.
     */
    sigset_t set;
    sigprocmask(0, NULL, &set);
    sigdelset(&set, code);
    sigprocmask(SIG_SETMASK, &set, 0);
#endif

    DieIfTooManyCAs();

    if (g_eTLSCrashState == NO_FAULT)
    {
        g_eTLSCrashState = IN_FAULT_HANDLER;

        if (g_pNumCrashAvoids)
        {
            HXAtomicIncUINT32(g_pNumCrashAvoids);
        }
        if (g_pNumCrashAvoidsInWindow)
        {
            HXAtomicIncUINT32(g_pNumCrashAvoidsInWindow);
        }

        GetCrashPrintLock();
#if defined _LINUX || defined _SOLARIS || defined _AIX
        dump_info(FALSE, g_eTLSCrashState, 0, code, pSiginfo, pSC);
#else
        dump_info(FALSE, g_eTLSCrashState, 0, code);
#endif // _LINUX
	
        ReleaseCrashPrintLock();

        g_eTLSCrashState = NO_FAULT;
        if (bRestartOnFault)
        {
            RestartOrKillServer();
        }

        ServerEngine* pEngine = get_my_engine();

        pEngine->CrashRecover();
        ASSERT(0);
    }
    else if (g_eTLSCrashState == IN_FAULT_HANDLER)
    {
        g_eTLSCrashState = IN_FAULT_DOUBLE_FAULT_HANDLER;

#if defined _LINUX || defined _SOLARIS || defined _AIX
        dump_info(
            FALSE, g_eTLSCrashState, "Double Fault, No Trace", code, pSiginfo, pSC);
#else
        dump_info(FALSE, g_eTLSCrashState, "Double Fault, No Trace", code);
#endif // _LINUX

        ReleaseCrashPrintLock(); // was locked in first crash handler
        g_eTLSCrashState = NO_FAULT;
        if (bRestartOnFault)
        {
            RestartOrKillServer();
        }

        ServerEngine* pEngine = get_my_engine();

        pEngine->CrashRecover();
        ASSERT(0);
    }
    else if (g_eTLSCrashState == IN_FAULT_DOUBLE_FAULT_HANDLER)
    {
        g_eTLSCrashState = IN_FAULT_DOUBLE_FAULT_HANDLER;
        g_ulTLSNumTripleFaults++;

        if (g_ulTLSNumTripleFaults > MAX_NUM_TRIPLE_FAULTS)
        {
            g_ulTLSNumTripleFaults = 0;
            goto JumpStart;
        }

        if (g_bCrashAvoidancePrint)
        {
            printf ("Triple Fault:  Directly JumpStarting Engine\n");
        }
        RSSManager::CrashOutput("Triple Fault: Directly JumpStarting Engine\n");

JumpStart:
        ReleaseCrashPrintLock(); // was locked in first crash handler

        g_eTLSCrashState = NO_FAULT;
        if (bRestartOnFault)
        {
            RestartOrKillServer();
        }

        ServerEngine* pEngine = get_my_engine();

#if defined _LINUX || defined _SOLARIS
        pEngine->JumpStart();
#else
        pEngine->mainloop(TRUE);
#endif // defined _LINUX || defined _SOLARIS

        ASSERT(0);
    }
}

#if defined PAULM_REPORT
int g_DoSocketTimes;
void _SocketTimes(int code)
{
    g_DoSocketTimes = 1;

#ifdef SHARED_FD_SUPPORT
    if (g_bSharedDescriptors)
    {
        char x;
        write(g_pProcessPipes[Process::get_procnum()][1], &x, 1);
        return;
    }
#endif /* SHARED_FD_SUPPORT */

    kill(getpid(), SIGUSR1);
}
#endif

BOOL g_bResetLeaks = FALSE;
void _MMresetleaks(int code)
{
    g_bResetLeaks = TRUE;

#ifdef SHARED_FD_SUPPORT
    if (g_bSharedDescriptors)
    {
        char x;
        write(g_pProcessPipes[Process::get_procnum()][1], &x, 1);
        return;
    }
#endif /* SHARED_FD_SUPPORT */

    kill(getpid(), SIGUSR1);
}

BOOL g_bCheckLeaks = FALSE;
void _MMcheckleaks(int code)
{
    g_bCheckLeaks = TRUE;

#ifdef SHARED_FD_SUPPORT
    if (g_bSharedDescriptors)
    {
        char x;
        write(g_pProcessPipes[Process::get_procnum()][1], &x, 1);
        return;
    }
#endif /* SHARED_FD_SUPPORT */

    kill(getpid(), SIGUSR1);
}

void _MMsuspendleaks(int code)
{
    printf("suspending leaks...\n");
    fflush(stdout);
    *g_pbTrackAllocs= FALSE;
}

void _MMcheckguards(int code)
{
    SharedMemory::checkguards();
}

void _MMreportsizes(int code)
{
    SharedMemory::reportsizes();
}

void _MMresetsizes(int code)
{
    SharedMemory::resetsizes();
}

void _MMstopsizes(int code)
{
    g_bDoSizes = FALSE;
}

BOOL g_bReconfigServer = FALSE;

void hup_handler(int code)
{
    g_bReconfigServer = TRUE;

#ifdef SHARED_FD_SUPPORT
    if (g_bSharedDescriptors)
    {
        char x;
        write(g_pProcessPipes[Process::get_procnum()][1], &x, 1);
        return;
    }
#endif /* SHARED_FD_SUPPORT */

    kill(getpid(), SIGUSR1);
}

void cont_handler(int code)
{
    perform_restart();
}

#endif // _UNIX

// platform-independent calls

void
RestartOnFault(BOOL bRestart)
{
    bRestartOnFault = bRestart;
}

BOOL
RestartOrKillServer()
{
    // restart, unless no-auto-restart requested.

    if (g_bNoAutoRestart)
    {
        ShutdownServer();
	exit(0);
    }

    return RestartServer();
}

#ifdef SUBSCRIPTION_INFO
void
DumpSubscriptionInfo(Process* pProc)
{
    int i=0;
    ServerEngine* pEngine=0;
    CHXMapLongToObj* pSubList = 0;
    Process* pProc2 = 0;
    Mutex* pSubListMutex = 0;
    for (i=0; i < MAX_THREADS; i++)
    {
        pProc2 = 0;
        pSubList = 0;
        pEngine = pProc->get_engine(i);
        if (pEngine)
        {
            pProc2 = (Process*)(pEngine->get_proc());
        }
        else
        {
            continue;
        }
        if (pProc2)
        {
            pSubList = pProc2->pc->subscriptions;
            pSubListMutex = pProc2->pc->subscriptions_lock;
        }
        if (pSubList && pSubListMutex)
        {
            pSubListMutex->Lock();
            LONG32 key=0;
            INT32* pValue=0;
            LISTPOSITION pos = pSubList->GetStartPosition();
            BOOL bPrinting = (BOOL)pos;
            if (pos)
            {
                //XXXDC I don't care for this output format but it'll do for now
                printf ("    ProcSubs[%d]: ", i);
            }
            while (pos)
            {
                pSubList->GetNextAssoc(pos, key, (void*&)pValue);
                if (*pValue)
                {
                    printf ("%ld:%ld ", key, *pValue);
                }
            }
            pSubListMutex->Unlock();
            if (bPrinting)
            {
                printf ("\n");
            }
        }
    }
}
#endif

OncePerSecond::OncePerSecond(Process* pProc)
{
    m_pProc = pProc;

    /* Wait 5 seconds for the server to settle */
    m_pProc->pc->engine->schedule.enter(m_pProc->pc->engine->now
        + Timeval(5.0), this);
}

STDMETHODIMP
OncePerSecond::Func()
{
    static int once = 0;
    m_pProc->pc->server_info->Func(m_pProc);
    m_pProc->pc->loadinfo->Func();

    m_pProc->pc->engine->schedule.enter(m_pProc->pc->engine->now
        + Timeval(1.0), this);
    if (!once)
    {
        /* XXXSMPNOW - The biggest hack on earth (XXXDC I made it bigger =) */
        once = 1;
        const char* szProdName = ServerVersion::ProductName();
        const char* szRelName = ServerVersion::ReleaseName();
        const char* szPlatform = ServerVersion::Platform();
        char* szMsg = new char[strlen(szProdName) + strlen(HX_COPYRIGHT_INFO) +
                               strlen(szRelName) + strlen(szPlatform) + 80];
        sprintf(szMsg, "%s %s", ServerVersion::ProductName(),
            HX_COPYRIGHT_INFO);
        m_pProc->pc->error_handler->Report(HXLOG_INFO, 0, 0, szMsg, 0);
        if (strlen(szRelName))
        {
           sprintf(szMsg, "Version: %s %s (%s)",
                   szProdName, szRelName, ServerVersion::VersionString());
        }
        else
        {
           sprintf(szMsg, "Version: %s %s (%s)",
                   szProdName, ServerVersion::MajorString(),
                   ServerVersion::VersionString());
        }
        m_pProc->pc->error_handler->Report (HXLOG_INFO, 0, 0, szMsg , 0);
        sprintf(szMsg, "Platform: %s", szPlatform);
        m_pProc->pc->error_handler->Report(HXLOG_INFO, 0, 0, szMsg, 0);
        delete[] szMsg;
    }

    return HXR_OK;
}

#ifdef REPORT_STACK_USAGE
char*
MemNChr(char* ptr, char c, UINT32 size)
{
    char* p = ptr;
    while (p - ptr < size)
    {
        if (*p != c)
            return p;
        p++;
    }

    return p;
}
#endif // REPORT_STACK_USAGE

RSSCoreStatsReport::RSSCoreStatsReport(Process *pProc) :
    m_pProc(pProc),
    m_pRSSManager(NULL),
    m_bFirstFire(true),
    m_ulRefCount(0),
    m_tLastTime(0, 0),
    m_ulLastBytesServed(0),
    m_ulLastForcedSelects(0),
    m_ulLastPPS(0),
    m_ulLastOverloads(0),
    m_ulLastNoBufs(0),
    m_ulLastOtherUDPErrs(0),
    m_ulLastNewClients(0),
    m_ulLastLeavingClients(0),
    m_ulLastMutexCollisions(0),
    m_ulLastMemOps(0),
    m_ulLastSchedulerItems(0),
    m_ulLastSchedulerItemsWithMutex(0),
#ifdef XXXAAKTESTRTSP
    m_ulLastOptionsMsg(0),
    m_ulLastDescribeMsg(0),
    m_ulLastSetupMsg(0),
    m_ulLastSetParameterMsg(0),
    m_ulLastPlayMsg(0),
    m_ulLastTeardownMsg(0),
#endif
#if ENABLE_LATENCY_STATS
    m_ulLastCorePassCBTime(0),
    m_ulLastCorePassCBCnt(0),
    m_ulLastCorePassCBMax(0),
    m_ulLastDispatchTime(0),
    m_ulLastDispatchCnt(0),
    m_ulLastDispatchMax(0),
    m_ulLastStreamerTime(0),
    m_ulLastStreamerCnt(0),
    m_ulLastStreamerMax(0),
    m_ulLastFirstReadTime(0),
    m_ulLastFirstReadCnt(0),
    m_ulLastFirstReadMax(0),
#endif
    m_ulLastCachedMallocs(0),
    m_ulLastCachedMisses(0),
    m_ulLastCachedNew(0),
    m_ulLastCachedDelete(0),
    m_ulLastShortTermAlloc(0),
    m_ulLastShortTermFree(0),
    m_ulLastPageFrees(0),
    m_ulLastPagesAllocated(0),
    m_ulLastFreeListEntriesSearched(0),
    m_ulLastBehind(0),
    m_ulLastResends(0),
    m_ulLastAggreg(0),
    m_ulLastWouldBlockCount(0),
    m_ulLastSocketAcceptCount(0),
    m_ulLastConcurrentOps(0),
    m_ulLastConcurrentMemOps(0),
    m_ulLastSchedulerElems(0),
    m_ulLastISchedulerElems(0),
    m_ulLastTotalNetReaders(0),
    m_ulLastTotalNetWriters(0),
    m_ulLastMutexNetReaders(0),
    m_ulLastMutexNetWriters(0),
    m_ulLastIdlePPMs(0),
    m_fLastBroadcastDistBytes(0),
    m_ulLastBroadcastDistPackets(0),
    m_ulLastBroadcastPacketsDropped(0),
    m_ulLastBroadcastPPMOverflows(0),
    m_ulLastFastBytesRead(0),
    m_ulLastSlowBytesRead(0),
    m_ulLastInternalBytesRead(0)
{
    memset(&m_LastBrcvStats, 0, sizeof(BrcvStatistics));
    memset(&m_LastBdstStats, 0, sizeof(BdstStatistics));
    memset(m_LastMainLoops, 0, sizeof(UINT32) * (MAX_THREADS+1));
    // Register w/ rss_manager
    HX_ASSERT(m_pProc->pc->rss_manager);
    m_pRSSManager = m_pProc->pc->rss_manager;
    m_pRSSManager->AddRef();

    m_pProc->pc->rss_manager->Register(this);
}

RSSCoreStatsReport::~RSSCoreStatsReport()
{
    m_pProc->pc->rss_manager->Remove(this);

    HX_RELEASE(m_pRSSManager);
}


STDMETHODIMP
RSSCoreStatsReport::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPropWatchResponse))
    {
        AddRef();
        *ppvObj = (IHXPropWatchResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
RSSCoreStatsReport::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
RSSCoreStatsReport::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP
RSSCoreStatsReport::Report(IHXLogOutput *pOutput, BOOL bEchoToStdout,
    HXTimeval tSkedNow)
{
    float timediff = (float)RSSManager::m_ulRSSReportInterval;
    char buffer[8192];
    char *pBufPointer = &(buffer[0]);

    Timeval tCurrentTime;

    tCurrentTime.tv_sec = tSkedNow.tv_sec;
    tCurrentTime.tv_usec = tSkedNow.tv_usec;

    if (m_tLastTime.tv_sec)
    {
        Timeval tTimeDiff = tCurrentTime - m_tLastTime;
        timediff = (float)tTimeDiff.tv_sec + (float)tTimeDiff.tv_usec / (1000 * 1000);
    }

    m_tLastTime = tCurrentTime;

    INT32 lTotalTrans =
                m_pProc->pc->server_info->m_TCPTransCount +
                m_pProc->pc->server_info->m_UDPTransCount +
                m_pProc->pc->server_info->m_MulticastTransCount;

    INT32 lTotalClient =
                m_pProc->pc->server_info->m_RTSPClientCount +
                m_pProc->pc->server_info->m_MMSClientCount +
                m_pProc->pc->server_info->m_HTTPClientCount;

    ServerInfo* pServerInfo = m_pProc->pc->server_info;

    UINT32 ulBytesServed = ServerInfo::CounterDifference(&m_ulLastBytesServed, *g_pBytesServed);
    UINT32 ulForcedSelects = ServerInfo::CounterDifference(&m_ulLastForcedSelects, *g_pForcedSelects);
    UINT32 ulPPS = ServerInfo::CounterDifference(&m_ulLastPPS, *g_pPPS);
    UINT32 ulOverloads = ServerInfo::CounterDifference(&m_ulLastOverloads, *g_pOverloads);
    UINT32 ulNoBufs = ServerInfo::CounterDifference(&m_ulLastNoBufs, *g_pNoBufs);
    UINT32 ulOtherUDPErrs = ServerInfo::CounterDifference(&m_ulLastOtherUDPErrs, *g_pOtherUDPErrs);
//    UINT32 ulMutexCollisions = ServerInfo::CounterDifference(&m_ulLastMutexCollisions, *g_pMutexCollisions);
    UINT32 ulConcurrentOps   = ServerInfo::CounterDifference(&m_ulLastConcurrentOps, *g_pConcurrentOps);
    UINT32 ulConcurrentMemOps = ServerInfo::CounterDifference(&m_ulLastConcurrentMemOps, *g_pConcurrentMemOps);
    UINT32 ulSchedulerElems = ServerInfo::CounterDifference(&m_ulLastSchedulerElems, *g_pSchedulerElems);
    UINT32 ulISchedulerElems = ServerInfo::CounterDifference(&m_ulLastISchedulerElems, *g_pISchedulerElems);
    UINT32 ulNewClients = ServerInfo::CounterDifference(&m_ulLastNewClients, pServerInfo->m_ulNewClients);
    UINT32 ulLeavingClients = ServerInfo::CounterDifference(&m_ulLastLeavingClients, pServerInfo->m_ulLeavingClients);


    UINT32 ulTotalMainLoops = 0;
    UINT32 MainLoops[MAX_THREADS+1];
    int i;
    for (i = 0; i < m_pProc->numprocs(); i++)
    {
        // Assign MainLoops[i] and increment ulTotalMainLoops
        ulTotalMainLoops += (MainLoops[i] = ServerInfo::CounterDifference(&(m_LastMainLoops[i]), g_pMainLoops[i]));
    }

#ifdef XXXAAKTESTRTSP
    UINT32 ulOptionsMsg = ServerInfo::CounterDifference(&m_ulLastOptionsMsg, pServerInfo->m_ulOptionsMsg);
    UINT32 ulDescribeMsg = ServerInfo::CounterDifference(&m_ulLastDescribeMsg, pServerInfo->m_ulDescribeMsg);
    UINT32 ulSetupMsg = ServerInfo::CounterDifference(&m_ulLastSetupMsg, pServerInfo->m_ulSetupMsg);
    UINT32 ulSetParameterMsg = ServerInfo::CounterDifference(&m_ulLastSetParameterMsg, pServerInfo->m_ulSetParameterMsg);
    UINT32 ulPlayMsg = ServerInfo::CounterDifference(&m_ulLastPlayMsg, pServerInfo->m_ulPlayMsg);
    UINT32 ulTeardownMsg = ServerInfo::CounterDifference(&m_ulTeardownMsg, pServerInfo->m_ulTeardownMsg);
#endif

#if ENABLE_LATENCY_STATS
    INT32 ulCorePassCBTime = ServerInfo::CounterDifference(&m_ulLastCorePassCBTime, pServerInfo->m_ulCorePassCBTime);
    INT32 ulCorePassCBCnt = ServerInfo::CounterDifference(&m_ulLastCorePassCBCnt, pServerInfo->m_ulCorePassCBCnt);
    INT32 ulCorePassCBMax = ServerInfo::CounterDifference(&m_ulLastCorePassCBMax, pServerInfo->m_ulCorePassCBMax);
    INT32 ulDispatchTime = ServerInfo::CounterDifference(&m_ulLastDispatchTime, pServerInfo->m_ulDispatchTime);
    INT32 ulDispatchCnt = ServerInfo::CounterDifference(&m_ulLastDispatchCnt, pServerInfo->m_ulDispatchCnt);
    INT32 ulDispatchMax = ServerInfo::CounterDifference(&m_ulLastDispatchMax, pServerInfo->m_ulDispatchMax);
    INT32 ulStreamerTime = ServerInfo::CounterDifference(&m_ulLastStreamerTime, pServerInfo->m_ulStreamerTime);
    INT32 ulStreamerCnt = ServerInfo::CounterDifference(&m_ulLastStreamerCnt, pServerInfo->m_ulStreamerCnt);
    INT32 ulStreamerMax = ServerInfo::CounterDifference(&m_ulLastStreamerMax, pServerInfo->m_ulStreamerMax);
    INT32 ulFirstReadTime = ServerInfo::CounterDifference(&m_ulLastFirstReadTime, pServerInfo->m_ulFirstReadTime);
    INT32 ulFirstReadCnt = ServerInfo::CounterDifference(&m_ulLastFirstReadCnt, pServerInfo->m_ulFirstReadCnt);
    INT32 ulFirstReadMax = ServerInfo::CounterDifference(&m_ulLastFirstReadMax, pServerInfo->m_ulFirstReadMax);
#endif


    UINT32 ulCachedMallocs = 0;
    UINT32 ulCachedMisses  = 0;
    UINT32 ulCachedNew     = 0;
    UINT32 ulCachedDelete  = 0;
    if ( m_pProc->pc->mem_cache)
    {
        m_pProc->pc->mem_cache->GetStats(&ulCachedMallocs,
            &ulCachedMisses, &ulCachedNew, &ulCachedDelete);
    }

    ulCachedMallocs = ServerInfo::CounterDifference(&m_ulLastCachedMallocs, ulCachedMallocs);
    ulCachedMisses  = ServerInfo::CounterDifference(&m_ulLastCachedMisses, ulCachedMisses);
    ulCachedNew     = ServerInfo::CounterDifference(&m_ulLastCachedNew, ulCachedNew);
    ulCachedDelete  = ServerInfo::CounterDifference(&m_ulLastCachedDelete, ulCachedDelete);



    pBufPointer += sprintf (pBufPointer, "Server Stats (");

    // TIME
    struct tm tm;
    HXTime now;
    gettimeofday(&now, 0);
    hx_localtime_r((const time_t *)&now.tv_sec, &tm);

    pBufPointer += strftime(pBufPointer, sizeof buffer, "%d-%b-%y %H:%M:%S)\n", &tm);

    pBufPointer += sprintf(pBufPointer, "    Uptime: %ld days, %02d:%02d:%02d\n",
        m_pProc->pc->server_info->Uptime() / (60 * 60 * 24),
        (m_pProc->pc->server_info->Uptime() / (60 * 60)) % 24,
        (m_pProc->pc->server_info->Uptime() / (60)) % (60),
        (m_pProc->pc->server_info->Uptime() % (60)));

    // player count, transport/protocol/cloaked breakdown
    long lRTSPPct=0, lMMSPct=0, lHTTPPct=0,
         lTCPPct=0, lUDPPct=0, lMCastPct=0, lCloakedPct=0;
    if (lTotalClient)
    {
        lRTSPPct    = 100 * m_pProc->pc->server_info->m_RTSPClientCount / lTotalClient;
        lMMSPct     = 100 * m_pProc->pc->server_info->m_MMSClientCount / lTotalClient;
        lHTTPPct    = 100 * m_pProc->pc->server_info->m_HTTPClientCount / lTotalClient;
        lCloakedPct = 100 * m_pProc->pc->server_info->m_CloakedClientCount / lTotalClient;
    }
    if (lTotalTrans)
    {
        lTCPPct     = 100 * m_pProc->pc->server_info->m_TCPTransCount / lTotalTrans;
        lUDPPct     = 100 * m_pProc->pc->server_info->m_UDPTransCount / lTotalTrans;
        lMCastPct   = 100 * m_pProc->pc->server_info->m_MulticastTransCount / lTotalTrans;
    }

    pBufPointer += sprintf(pBufPointer, 
           "    Players: %ld (New Players: %d, %0.2f/sec,"
           " Players Leaving: %d, %0.2f/sec)\n",
           m_pProc->pc->server_info->m_ClientCount, 
           ulNewClients, ulNewClients / timediff,
           ulLeavingClients, ulLeavingClients / timediff);
    pBufPointer += sprintf(pBufPointer, "    Players by Protocol: %ld%% RTSP, %ld%% MMS, %ld%% "
           "HTTP (%ld%% Cloaked)\n", lRTSPPct, lMMSPct, lHTTPPct,
           lCloakedPct);
    pBufPointer += sprintf(pBufPointer, "    Players by Transport: %ld%% TCP, %ld%% UDP, %ld%% MCast\n",
           lTCPPct, lUDPPct, lMCastPct);
    pBufPointer += sprintf(pBufPointer, "    Net Devices: %ld\n", m_pProc->pc->server_info->m_MidBoxCount);

#if XXXAAKTESTRTSP
    pBufPointer += sprintf(pBufPointer"    RTSP Stats: O(%lu)+D(%lu)+S(%lu)+SP(%lu)+P(%lu)+T(%lu)"
        " = Total(%lu)\n", ulOptionsMsg, ulDescribeMsg, ulSetupMsg,
        ulSetParameterMsg, ulPlayMsg, ulTeardownMsg,
        (ulOptionsMsg + ulDescribeMsg + ulSetupMsg + ulSetParameterMsg +
        ulPlayMsg + ulTeardownMsg));
#endif

#if ENABLE_LATENCY_STATS
    pBufPointer += sprintf(pBufPointer, "    Latency Stats: CorePassCB %lu (Avg %lu, Max %lu); "
            "Dispatch %lu (Avg %lu, Max %lu)\n",
        ulCorePassCBCnt,
        ulCorePassCBCnt ? ulCorePassCBTime/ulCorePassCBCnt : 0,
        ulCorePassCBMax,
        ulDispatchCnt,
        ulDispatchCnt ? ulDispatchTime/ulDispatchCnt : 0,
        ulDispatchMax);
    pBufPointer += sprintf(pBufPointer, "    Latency Stats: Streamer %lu (Avg %lu, Max %lu); "
            "FirstRead %lu (Avg %lu, Max %lu)\n",
        ulStreamerCnt,
        ulStreamerCnt ? ulStreamerTime/ulStreamerCnt : 0,
        ulStreamerMax,
        ulFirstReadCnt,
        ulFirstReadCnt ? ulFirstReadTime/ulFirstReadCnt : 0,
        ulFirstReadMax);
#endif

    // Memory Usage
    UINT32 ulBytesInUse = SharedMemory::BytesInUse();
    UINT32 ulBytesAllocated = SharedMemory::BytesAllocated();
    UINT32 ulShortTermAlloc = ServerInfo::CounterDifference(&m_ulLastShortTermAlloc, *g_pShortTermAlloc);
    UINT32 ulShortTermFree  = ServerInfo::CounterDifference(&m_ulLastShortTermFree, *g_pShortTermFree);
    UINT32 ulPageFrees      = ServerInfo::CounterDifference(&m_ulLastPageFrees, *g_pPageFrees);
    UINT32 ulPagesAllocated = ServerInfo::CounterDifference(&m_ulLastPagesAllocated, *g_pPagesAllocated);
    UINT32 ulFreeListEntriesSearched = ServerInfo::CounterDifference(&m_ulLastFreeListEntriesSearched,
                                                         *g_pFreeListEntriesSearched);


    UINT32 ulTotalAllocs = ulShortTermAlloc + ulCachedMallocs;
    pBufPointer += sprintf(pBufPointer, "    Memory Stats: %lu Bytes In Use (Allocation Cache Hit Ratio %ld%%)\n",
        ulBytesInUse,
        ulTotalAllocs ?  100 * ulCachedMallocs / ulTotalAllocs : 0);

    pBufPointer += sprintf(pBufPointer, "    Memory Alloc: %lu Bytes (%ld Pages); MMapIO: %lu Bytes\n",
       ulBytesAllocated, ulBytesAllocated / 4096,
       *g_pMemoryMappedDataSize);

    pBufPointer += sprintf(pBufPointer, "    Recent Memory Stats: %ld Mallocs, %ld Frees, %ld CacheMalloc %ld CacheMiss\n",
        ulShortTermAlloc, ulShortTermFree, ulCachedMallocs, ulCachedMisses);

    pBufPointer += sprintf(pBufPointer, "    Recent Memory Stats: %ld CachedNew, %ld CachedDel\n", ulCachedNew, ulCachedDelete);

    pBufPointer += sprintf(pBufPointer, "    Recent Memory Stats: %ld PageFrees (%0.2f%%), %ld PageAllocs (%0.2f%%)\n",
        ulPageFrees, ulShortTermFree ? ulPageFrees * 100.0 / ulShortTermFree : 0,
        ulPagesAllocated, ulShortTermAlloc ? ulPagesAllocated * 100.0 / ulShortTermAlloc : 0);

    pBufPointer += sprintf(pBufPointer, "    %d Second Memory Stats: %ld FreeListEntriesSearched (%0.2f per Malloc)\n",
        RSSManager::m_ulRSSReportInterval, ulFreeListEntriesSearched,
        ulShortTermAlloc ? (double)ulFreeListEntriesSearched / (double)ulShortTermAlloc : 0);


    float fAllocOverhead = ulBytesInUse ?
      (ulBytesAllocated - *g_pFreePagesOutstanding * _PAGESIZE) * 100.0f / ulBytesInUse - 100.0f :
      0;
    pBufPointer += sprintf(pBufPointer, "    Memory Allocated OverHead: %ld Free Pages Outstanding, %0.1f%% Overhead\n",
        *g_pFreePagesOutstanding, fAllocOverhead);

    float fMemPerPlayer = lTotalClient ? ulBytesInUse / (float)lTotalClient / 1024.0f : 0;
    pBufPointer += sprintf(pBufPointer, "    Memory per Player: %0.0fk\n", fMemPerPlayer);

    /* bandwidth vars */
    double fOutputMbps = ulBytesServed / timediff * 8.0 / 1024.0 / 1000.0;

    double fOutputPerPlayerKbps = (!lTotalClient) ? 0 :
        ulBytesServed / timediff * 8.0 / 1024.0 / lTotalClient;

    double fPctSubscribed = (!m_pProc->pc->server_info->m_BandwidthUsage) ? 100 :
        ulBytesServed / timediff * 8.0 * 100 / m_pProc->pc->server_info->m_BandwidthUsage;

    double fSubscribedMbps = m_pProc->pc->server_info->m_BandwidthUsage / 1024.0 / 1000.0;

    double fPctNominal = (!(*g_pAggregateRequestedBitRate)) ? 100 :
        ulBytesServed / (*g_pAggregateRequestedBitRate * timediff / 8.0) * 100;

    double fNominalMbps = *g_pAggregateRequestedBitRate / 1024.0 / 1000.0;

    // Banwidth Usage
    pBufPointer += sprintf(pBufPointer, "    Bandwidth Stats: Output %0.2f Mbps, %0.1f Kbps Per Player\n"
            "    Bandwidth Stats: %0.0f%% Subscribed (%0.2f Mbps), %0.0f%% Nominal (%0.2f Mbps)\n",
            fOutputMbps, fOutputPerPlayerKbps,
            fPctSubscribed, fSubscribedMbps, fPctNominal, fNominalMbps);

    UINT32 ulBehind  = ServerInfo::CounterDifference(&m_ulLastBehind, *g_pBehind);
    UINT32 ulResends = ServerInfo::CounterDifference(&m_ulLastResends, *g_pResends);
    UINT32 ulAggreg  = ServerInfo::CounterDifference(&m_ulLastAggreg, *g_pAggreg);
    UINT32 ulWouldBlockCount = ServerInfo::CounterDifference(&m_ulLastWouldBlockCount, *g_pWouldBlockCount);
    UINT32 ulSocketAcceptCount = ServerInfo::CounterDifference(&m_ulLastSocketAcceptCount, *g_pSocketAcceptCount);


    pBufPointer += sprintf(pBufPointer, "    Misc Recent Stats: Packets %ld, Overload %ld, NoBufs %ld, OtherUDPErrs %ld, Behind %ld\n",
        ulPPS, ulOverloads, ulNoBufs, ulOtherUDPErrs, ulBehind);

    pBufPointer += sprintf(pBufPointer, "    Misc Recent Stats: Resend %ld, AggregatedPkts %ld\n",
        ulResends, ulAggreg);

    pBufPointer += sprintf(pBufPointer, "    Misc Recent Stats: WouldBlocks %ld, Accepts %ld (%0.2f/sec)\n",
        ulWouldBlockCount, ulSocketAcceptCount, ulSocketAcceptCount / timediff);

    pBufPointer += sprintf(pBufPointer, "    Mutex Collisions: %ld / sec, ~%0.3f%% CPU Spinning, ~%0.3f%% Memory Ops\n",
        (int)(ulConcurrentOps / timediff),
        (ulConcurrentOps / timediff / (12000000.0 / 100.0)),
        (float)ulConcurrentMemOps * 100 / (float)(ulConcurrentOps + 1));

    UINT32 ulTotalNetReaders = ServerInfo::CounterDifference(&m_ulLastTotalNetReaders, *g_pTotalNetReaders);
    UINT32 ulMutexNetReaders = ServerInfo::CounterDifference(&m_ulLastMutexNetReaders, *g_pMutexNetReaders);
    UINT32 ulTotalNetWriters = ServerInfo::CounterDifference(&m_ulLastTotalNetWriters, *g_pTotalNetWriters);
    UINT32 ulMutexNetWriters = ServerInfo::CounterDifference(&m_ulLastMutexNetWriters, *g_pMutexNetWriters);


    if (*g_bLimitParallelism)
    {
        pBufPointer += sprintf(pBufPointer, "    Scheduler Items: %d (With Mutex), %d (Without Mutex)\n",
            ulSchedulerElems + ulISchedulerElems, 0);
        pBufPointer += sprintf(pBufPointer, "    Network Items: Read %d Write %d | ThreadSafe Read 0 (0.00%%) | ThreadSafe Write 0 (0.00%%)\n",
                ulTotalNetReaders, ulTotalNetWriters);
    }
    else
    {
        pBufPointer += sprintf(pBufPointer, "    Scheduler Items: %d (With Mutex), %d (Without Mutex)\n",
                               ulSchedulerElems, ulISchedulerElems);

        double fMutexReadPct = 0.0;
        if (ulTotalNetReaders)
        {
            fMutexReadPct = 100.0 * ulMutexNetReaders /
                (ulTotalNetReaders + ulMutexNetReaders);
        }
        double fMutexWritePct = 0.0;
        if (ulTotalNetWriters)
        {
            fMutexWritePct = 100.0 * ulMutexNetWriters /
                (ulTotalNetWriters + ulMutexNetWriters);
        }
        pBufPointer += sprintf(pBufPointer, "    Network Items: Read %d (%0.2f%% With Mutex), Write %d (%0.2f%% With Mutex)\n",
                ulTotalNetReaders,
                fMutexReadPct,
                ulTotalNetWriters,
                fMutexWritePct);
    }

    UINT32 ulIdlePPMs    = ServerInfo::CounterDifference(&m_ulLastIdlePPMs, *g_pIdlePPMs);

    pBufPointer += sprintf(pBufPointer, "    Misc: %d File Objs, %d IdlePPMs, %d Forced Selects, %d AggSupport, %d:%d CAs\n",
        *g_pFileObjs, ulIdlePPMs, ulForcedSelects,
        *g_pAggregatablePlayers, *g_pNumCrashAvoids, *g_pNumCrashAvoidsInWindow);


    double fBroadcastDistBytes = ServerInfo::CounterDifference(&m_fLastBroadcastDistBytes, *g_pBroadcastDistBytes);

    double scale = 0;

    if (pServerInfo->m_pBroadcastRecvrStats)
    {
        UINT32 uBytesRcvd;
        UINT32 uPacketsRcvd;
        UINT32 uOutOfOrder;
        UINT32 uLost;
        UINT32 uLate;
        UINT32 uResendsRequested;
        UINT32 uDuplicates;
        UINT32 uLostUpstream;

        BrcvStatistics* bs = pServerInfo->m_pBroadcastRecvrStats;

        uPacketsRcvd = ServerInfo::CounterDifference(&(m_LastBrcvStats.m_uPacketsRcvd),
                                                     bs->m_uPacketsRcvd);
        uBytesRcvd = ServerInfo::CounterDifference(&(m_LastBrcvStats.m_uBytesRcvd),
                                                   bs->m_uBytesRcvd);
        uLost = ServerInfo::CounterDifference(&(m_LastBrcvStats.m_uLost),
                                              bs->m_uLost);
        uLate = ServerInfo::CounterDifference(&(m_LastBrcvStats.m_uLate),
                                              bs->m_uLate);
        uLostUpstream = ServerInfo::CounterDifference(&(m_LastBrcvStats.m_uLostUpstream),
                                                      bs->m_uLostUpstream);
        uResendsRequested = ServerInfo::CounterDifference(&(m_LastBrcvStats.m_uResendsRequested),
                                                          bs->m_uResendsRequested);
        uOutOfOrder = ServerInfo::CounterDifference(&(m_LastBrcvStats.m_uOutOfOrder),
                                                    bs->m_uOutOfOrder);
        uDuplicates = ServerInfo::CounterDifference(&(m_LastBrcvStats.m_uDuplicates),
                                                    bs->m_uDuplicates);

        scale = uBytesRcvd * 8.0 / timediff / 1000.0;

        pBufPointer += sprintf(pBufPointer, "    Broadcast Reception: Feeds %ld, %0.2f Kbps, Packets %ld, Lost %ld, Lost Upstream %ld\n",
            bs->m_uReceiverSessions,
            scale,
            uPacketsRcvd,
            uLost,
            uLostUpstream);
        pBufPointer += sprintf(pBufPointer, "    Broadcast Reception: Resends %ld, Out of Order %ld, Duplicate %ld, Late %ld\n",
            uResendsRequested,
            uOutOfOrder,
            uDuplicates,
            uLate);
    }
    else
    {
        pBufPointer += sprintf(pBufPointer, "    Broadcast Reception: Inactive\n");
    }

    if (pServerInfo->m_pBroadcastDistStats)
    {
        UINT32 uBytesSent;
        UINT32 uPacketsSent;

        BdstStatistics* bs = pServerInfo->m_pBroadcastDistStats;

        uPacketsSent = ServerInfo::CounterDifference(&(m_LastBdstStats.m_uPacketsSent),
                                                     bs->m_uPacketsSent);
        uBytesSent = ServerInfo::CounterDifference(&(m_LastBdstStats.m_uBytesSent),
                                                   bs->m_uBytesSent);

        scale = uBytesSent * 8.0 / timediff / 1000.0;

        pBufPointer += sprintf(pBufPointer, "    Broadcast Distribution: Feeds %ld (Push %ld, Pull %ld), %0.2f Kbps, Packets %ld\n",
            bs->m_uDistSessions,
            bs->m_uPushSessions,
            bs->m_uPullSessions,
            scale,
            uPacketsSent);
        pBufPointer += sprintf(pBufPointer, "    Broadcast Distribution: Resends %ld (Requested %ld), Lost Upstream %ld, Dropped %ld\n",
            bs->m_uResendsHonored,
            bs->m_uResendsRequested,
            bs->m_uLostUpstream,
            bs->m_uBufferOverruns);
    }
    else
    {
        pBufPointer += sprintf(pBufPointer, "    Broadcast Distribution: Inactive\n");
    }

    UINT32 ulBroadcastPacketsDropped = ServerInfo::CounterDifference(&m_ulLastBroadcastPacketsDropped,
                                                         *g_pBroadcastPacketsDropped);
    UINT32 ulBroadcastPPMOverflows   = ServerInfo::CounterDifference(&m_ulLastBroadcastPPMOverflows,
                                                         *g_pBroadcastPPMOverflows);

    pBufPointer += sprintf(pBufPointer, "    Broadcast Core: %ld Dropped Packets, %ld Client Overflows\n",
            ulBroadcastPacketsDropped, ulBroadcastPPMOverflows);


    // FastFile Stats
    double fFFEffect = 0;
    UINT32 ulBlockCount = 0,
           ulInUseBlockCount = 0,
           ulFobCount = 0,
           ulFastBytesRead = 0,
           ulSlowBytesRead = 0,
           ulInternalBytesRead = 0,
           ulMemUse = 0;

    if (HXR_OK == FastFileStats::GetStats (m_pProc->pc->server_context,
                                           &fFFEffect,
                                           &ulBlockCount,
                                           &ulInUseBlockCount,
                                           &ulFobCount,
                                           &ulFastBytesRead,
                                           &ulSlowBytesRead,
                                           &ulInternalBytesRead,
                                           &ulMemUse))
    {
        ulFastBytesRead = ServerInfo::CounterDifference(&m_ulLastFastBytesRead, ulFastBytesRead);
        ulSlowBytesRead = ServerInfo::CounterDifference(&m_ulLastSlowBytesRead, ulSlowBytesRead);
        ulInternalBytesRead = ServerInfo::CounterDifference(&m_ulLastInternalBytesRead, ulInternalBytesRead);

        pBufPointer += sprintf(pBufPointer, "    FastFile: %0.2f%% Effective, "
                        "%lu Blocks (%lu), %lu Fobs\n",
                        fFFEffect,
                        ulBlockCount,
                        ulInUseBlockCount,
                        ulFobCount);

        pBufPointer += sprintf(pBufPointer, "    FastFile: Bytes: %lu Fast, "
                        "%lu Slow, %lu Internal %lu Total\n",
                        ulFastBytesRead,
                        ulSlowBytesRead,
                        ulInternalBytesRead,
                        ulMemUse);
    }


    const char* pHealth;
    switch (m_pProc->pc->loadinfo->GetLoadState())
    {
        case NormalLoad:
            pHealth = "0";
            break;
        case HighLoad:
            pHealth = "1";
            break;
        case ExtremeLoad:
            pHealth = "2";
            break;
        default:
            //Unknown
            pHealth = "?";
            break;
    }

    pBufPointer += sprintf(pBufPointer, "    CPU Usage: %d%% User, %d%% Kernel, %d%% System\n",
        m_pProc->pc->server_info->m_PercentUserCPUUsage,
        m_pProc->pc->server_info->m_PercentKernCPUUsage,
        m_pProc->pc->server_info->m_TotalCPUUsage);

    pBufPointer += sprintf(pBufPointer, "    Main Loop Iterations: %ld/sec (LoadState: %s)\n",
        (int)(ulTotalMainLoops / timediff), pHealth);

    pBufPointer += sprintf(pBufPointer, "    MainLoopIts:   ");
    for (i = 0; i < m_pProc->numprocs(); i++)
    {
        pBufPointer += sprintf(pBufPointer, " %6ld", MainLoops[i]);
    }
    pBufPointer += sprintf(pBufPointer, "\n");

    pBufPointer += sprintf(pBufPointer, "    Registered FDs:");
    ServerEngine* pServerEngine;
    for (i = 0; i < m_pProc->numprocs(); i++)
    {
        pServerEngine = m_pProc->get_engine(i);
        pBufPointer += sprintf(pBufPointer, " %6ld", (pServerEngine ? pServerEngine->NumFDs() : 0));
    }
    pBufPointer += sprintf(pBufPointer, "\n");

#if XXXAAK_SOLARIS_COUNT_PROC_FDS
    pBufPointer += sprintf(pBufPointer, "    Actual FDs:    ");
    int num_fds = 0;
    for (i = 0; i < m_pProc->numprocs(); i++)
    {
        num_fds = 0;
        struct dirent* de = 0;
        char dir_to_open[32];
        sprintf(dir_to_open, "/proc/%d/fd", m_pProc->procid(i));
        DIR* dp = opendir(dir_to_open);
        if (!dp)
        {
            pBufPointer += sprintf(pBufPointer, " %4ld", num_fds);
            continue;
        }
        struct dirent pDEList[NAME_MAX+1];
        struct dirent* pDERes = 0;
        while (de = hx_readdir_r(dp, pDEList, &pDERes))
            num_fds++;
        closedir(dp);
        pBufPointer += sprintf(pBufPointer, " %4ld", num_fds);

    }
    pBufPointer += sprintf(pBufPointer, "\n");
#endif /* XXXAAK_SOLARIS_COUNT_PROC_FDS */

#ifdef DISPATCHQ_RSS
    m_pProc->pc->dispatchq->printCounters(m_pProc, pBufPointer);
#endif

#ifdef REPORT_STACK_USAGE
    pBufPointer += sprintf(pBufPointer, "    Stack Usage:    %4ld", 0);

    for (UINT32 i = 1; i < MAX_THREADS; i++)
    {
        if (!g_threadStacks[i].pStack)
            break;

        char* pStack = g_threadStacks[i].pStack + GUARD_SIZE + 1;
        char* pEnd = (char*)MemNChr(pStack, 0xff, STACKSIZE - GUARD_SIZE);
        g_threadStacks[i].ulUsage = STACKSIZE - (size_t)(pEnd - g_threadStacks[i].pStack);
        pBufPointer += sprintf(pBufPointer, " %4ld", g_threadStacks[i].ulUsage);
        fflush(stdout);
    }

    pBufPointer += sprintf(pBufPointer, "\n");
#endif // REPORT_STACK_USAGE

#ifdef SUBSCRIPTION_INFO
    DumpSubscriptionInfo(m_pProc);
#endif

    pBufPointer += sprintf(pBufPointer, "    Elapsed Time: %0.2f\n", timediff);

    if (*g_pSecondsToShutdown)
    {
        pBufPointer += sprintf(pBufPointer, "    Server Shutting Down\n");
        pBufPointer += sprintf(pBufPointer, "    Time remaining until shutdown: %d seconds\n",
                        *g_pSecondsToShutdown - tCurrentTime.tv_sec);
    }

    if (RSSManager::m_bRSSReportEnabled)
    {
        printf("%s", buffer);
        fflush(stdout);
    }

    HX_RESULT retVal = pOutput->Output(buffer);
    if (FAILED(retVal))
    {
        printf("RSS log error: ");
        LogFileOutput* pLFO = dynamic_cast<LogFileOutput*>(pOutput);
        if (pLFO)
        {
            printf("(%lu) %s\n", pLFO->GetLastErrorCode(), pLFO->GetLastErrorStr());
        }
        else
        {
            printf("No additional information available.\n");
        }
    }


    return retVal;
}

#ifdef _UNIX
void
monitor_child_handler(int code, siginfo_t* pSigInfo, ucontext_t* pSC)
{
    int status;
    pid_t childpid;
    BOOL bChildExitedWithRestartSignal = FALSE;
    static BOOL bShutdownStarted = FALSE;

    while ((childpid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (g_bShowDebugErrorMessages)
        {
	    int pid = getpid();
            if (WIFEXITED(status))
            {
                printf("%d: SIGCHLD received, pid %d exited(%d): "
                       "child exit with status %d\n", pid,
                       (int)childpid, status, WEXITSTATUS(status));
            }
            else if (WIFSIGNALED(status))
            {
                printf("%d: SIGCHLD received, pid %d status signaled(%d): "
                       "child exited due to unhandled signal %d)\n",
                       pid, (int)childpid, status, WTERMSIG(status));
            }
            else
            {
                printf("%d: SIGCHLD received, pid %d status unknown(%d)\n",
                    pid, (int)childpid, status);
            }
            fflush(0);
        }
	if (WIFEXITED(status) && WEXITSTATUS(status) == RESTART_SIGNAL)
	{
	    bChildExitedWithRestartSignal = TRUE;
	}
    }
    if (bChildExitedWithRestartSignal || (!g_bNoAutoRestart && g_bForceRestart))
    {
	if (!bShutdownStarted)
	{
    write(MonitorControllerSocket[1], "R", 1);
	    bShutdownStarted = TRUE;
	}
    }
    else 
    {
	if (!bShutdownStarted)
	{
	    write(MonitorControllerSocket[1], "S", 1);
	    bShutdownStarted = TRUE;
	}
    }
}

static int monitor_reported_controller_pid = 0;
static int alive_restart = 0;

void
monitor_pass_handler(int code)
{
    kill(monitor_reported_controller_pid, code);
}

void
monitor_alive_restarter(int code)
{
    alive_restart = 1;
    kill(monitor_reported_controller_pid, RESTART_SIGNAL);
}

void
monitor_dump_trace(int code)
{
    kill(monitor_reported_controller_pid, SIGXFSZ);
#ifndef _SOLARIS
    char *pTrace = get_trace();

    CA_PRINT(("\n------------------------ %d -------------------------\n", getpid()));
    CA_PRINT(("%s", pTrace));
#else
    char str[128];
    sprintf(str, "/usr/proc/bin/pstack %d\n", getpid());
    system(str);
#endif
}

void
alive_checker_exit(int code)
{
    exit(-1);
}
#endif // _UNIX

#if defined(_AIX) || defined (_HPUX) || defined(_LINUX) || \
    defined(_SOLARIS) || defined(_OSF1) || defined(_WINDOWS) || \
    defined(_FREEBSD) || defined(_OPENBSD) || defined(_NETBSD)
int CPUDetect()
{
    UINT32 nnumProc = g_pSysInfo -> GetNumberofCPU();
    suprintf("%d CPU%s Detected...\n", nnumProc, (nnumProc > 1) ? "s" : "");
    return nnumProc;
}
#elif defined _UNIX
int CPUDetect()
{
    UINT32 SingleCpuWork = 0;
#ifdef _LINUX
    BYTE *a;
    INT32 f1, f2;

    f1 = open("/dev/zero", O_RDWR);

    a = (BYTE*)mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, f1, 0);

    f2 = open("/proc/self/mem", O_RDWR);

    UINT32* pTotalWork = (UINT32*)mmap(0, 4096, PROT_READ | PROT_WRITE,
                 MAP_SHARED, f2, (off_t)a);

    munmap(a, 4096);
    close(f1);
    close(f2);
#else
    UINT32* pTotalWork = new UINT32;
#endif
    int headpid = getpid();
    int times = 0;
    HX_MUTEX pMutex;
    struct passwd* pPwEnt = getpwuid(geteuid());
    const char* pUsername = pPwEnt ? pPwEnt->pw_name : 0;

    pMutex = HXCreateMutex();

    suprintf("Detecting Number of CPUs...\n");

    sleep(1);

    while (times < 16)
    {
        *pTotalWork = 0;
        printf ("   Testing %d CPU(s): ", times + 1);
        fflush(0);
        for (int j = 0; j < times; j++)
        {
            if (getpid() == headpid)
            {
                if (fork() < 0)
                {
                    switch(errno)
                    {
                        case EAGAIN:
                            suprintf("<%s> has too many forked processes!\n",
                                pUsername ? pUsername : "User");
                            break;
                        case ENOMEM:
                            suprintf("fork() failed due to INSUFFICIENT SWAP "
                                "space!\n");
                            break;
                        default:
                            suprintf("fork() failed!\n");
                            break;
                    }
                    fflush(0);
                }
            }
        }
        UINT32 ulCurrentTime = HX_GET_TICKCOUNT();
        UINT32 ulElapsedTime = 0;

        volatile int i;
        for (i = 1; ulElapsedTime < 2500; i++)
        {
            volatile int y = 10 * i;
            volatile int z = 10 * i;
            volatile int a = 137850 / i;
            volatile int b = 132890 / i;
            volatile float a1 = 51874.2000 / (float)i * 0.22;
            if (i % 10000 == 0)
                ulElapsedTime =
                    CALCULATE_ELAPSED_TICKS(ulCurrentTime, HX_GET_TICKCOUNT());
        }

        HXMutexLock(pMutex);
        *pTotalWork += i;
        HXMutexUnlock(pMutex);
        if (times == 0 && headpid == getpid())
        {
            SingleCpuWork = i;
            printf ("1 CPU Detected, Phew...\n");
        }
        else if (headpid == getpid())
        {
            sleep(1);
            if (*pTotalWork > (SingleCpuWork * (times + 1)) -
                SingleCpuWork * 0.25)
            {
                printf ("%d CPUs Detected (%0.0f%% Work Produced)\n",
                    times + 1, 100 * *pTotalWork / (float)SingleCpuWork);
            }
            else
            {
                printf ("%d CPUs Not Detected (%0.0f%% Work Produced)\n",
                    times + 1, 100 * *pTotalWork / (float)SingleCpuWork);

                return times;
            }
        }
        else
        {
            shared_ready = 0;
            fflush(0);
            _exit(0);
        }
        times++;
    }

#ifdef _LINUX
    munmap((char *)pTotalWork, 4096);
#else
    delete pTotalWork;
#endif
    HXDestroyMutex(pMutex);
}
#endif

int
GetStreamerCount(Process* proc)
{
#ifdef _SOLARIS
	/*
	 * pr# 178507.
	 * this fix is mainly for the UltraSparc T1 chip where the server is
	 * going to spawn as many streamers as there are cores in the chip and
	 * as of 9/25/2006 Sun only has 2 platforms which have this chip - the
	 * SUN-Fire-T1000 and the SUN-Fire-T2000. for some reason the T2000 is
	 * represented as SUN-Fire-T200 as the platform string in sysinfo.
	 *
	 * this code kicks in only when the NumStreamers var is not present in
	 * the server config file.
	 */
	char pStr[256];
	long lStrLen = 256;
	if (sysinfo(SI_PLATFORM, pStr, lStrLen) < 0)
	{
	    LOGMSG(proc->pc->error_handler, HXLOG_INFO, "sysinfo(SI_PLATFORM) failed: %s\n", strerror(errno));
	    LOGMSG(proc->pc->error_handler, HXLOG_INFO, "Streamer count same as Processor count(%d)\n", *g_pCPUCount);
	    return *g_pCPUCount;
	}
	else if ((!strncasecmp(pStr, "SUNW,Sun-Fire-T1000", lStrLen)) ||
	    (!strncasecmp(pStr, "SUNW,Sun-Fire-T200", lStrLen)))
	{
	    /*
	     * assuming that these two models have the UltraSparc T1 chip
	     *
	     * UltraSparc T1 cip can have upto 8 cores. Each core has
	     * 4 coolthreads per core. For now the server starts off only 1
	     * Streamer per core, because the server/coolthread scalability is
	     * linear from 1 to number-of-cores.
	     */
	    long ulCPUsOnline = sysconf(_SC_NPROCESSORS_ONLN);
	    long ulCPUsConfigured = sysconf(_SC_NPROCESSORS_CONF);
	    long ulCPUsMax = sysconf(_SC_NPROCESSORS_MAX);
	    long ulNumCores = ulCPUsConfigured/4; // 4 coolthreads/core
	    return ulNumCores;
	}
	else
	    return *g_pCPUCount;
#else
        return *g_pCPUCount;
#endif // _SOLARIS
}

#ifdef _WIN32
void AliveChecker(SocketPair* pMonitorAliveChecker);

DWORD
AliveCheckerThread(void* p)
{
    AliveChecker((SocketPair*)p);
    /* Never to return */
    HX_ASSERT(0);
    return 0;
}
#endif

#define ALIVE_CHECK_INTERVAL 45

void
AliveChecker(SocketPair* pMonitorAliveChecker)
{
#ifdef _UNIX
    setsid();
    struct rlimit r = {0, 0};
    setrlimit(RLIMIT_CORE, &r);
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))alive_checker_exit;
    sigaction(SIGUSR1, &sa, NULL);
    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
#endif
    UINT32 ulPort = 0;
    UINT32 ulFailures = 0;
    BOOL bEverSucceeded = FALSE;
    BOOL bDisable = FALSE;

    while(1)
    {
#ifdef _UNIX
        sleep(ulFailures && ulFailures <= 3 ? 3 : ALIVE_CHECK_INTERVAL);
#else
        Sleep((ulFailures && ulFailures <= 3 ? 3 : ALIVE_CHECK_INTERVAL) * 1000);
#endif
        TCPIO TcpIO;

        UINT8 temp[1024];
        int nread;
        nread = pMonitorAliveChecker->Read(temp, 1024);
        UINT8* ptr = temp;

        while (nread >= 5)
        {
            char Opcode = *ptr;
            UINT32 ulValue = getlong(ptr + 1);
            nread -= sizeof (UINT32) + 1;
            ptr += sizeof (UINT32) + 1;
            switch(Opcode)
            {
            case 'P':
                ulPort = ulValue;
                //printf ("*** Alive Checker Received New Port Number: %d\n", ulPort);
                break;
#ifdef _UNIX
            case 'G':
#if defined(_AIX) || defined( _HPUX)
                if (setregid(-1, ulValue) < 0)
#else
                if (setegid(ulValue) < 0)
#endif
                    perror("setegid() failed(3)");
                //printf ("*** Alive Checker Received GID: %d\n", ulValue);
                break;
            case 'U':
#if defined(_AIX) || defined(_HPUX)
                if (setreuid(-1, ulValue) < 0)
#else
                if (seteuid(ulValue) < 0)
#endif
                    perror("setugid() failed(3)");
                //printf ("*** Alive Checker Received UID: %d\n", ulValue);
                break;
#endif
            case 'D':
                bDisable = TRUE;
                //printf ("*** Alive Checker Disabled\n");
                break;
            default:
                //printf ("*** Alive Checker Wrong Opcode! %c\n", Opcode);
                break;
            }
        }

        if (bDisable == FALSE && ulPort)
        {
            char buf[1024];
            int n = 0;
            UINT32 ulStep = 0;
	    int error = 0;

            //printf ("*** Alive Checker Start Run\n");
            if (g_bShowDebugErrorMessages)
            {
                printf ("* Starting Alive Checker HeartBeat Test\n");
                fflush(0);
            }
            if (TcpIO.init(INADDR_ANY, 0, 0, 0, 0) < 0)
		error = TcpIO.error();
	    if (error) goto fail;
            ulStep++;

            TcpIO.connect(g_pHeartBeatIP ? g_pHeartBeatIP : (char*)"127.0.0.1",
                          (INT16)ulPort);
#ifdef _UNIX
            sleep(10); // Wait for Connect
#else
            Sleep( (ulFailures <= 1 ? 10 : 30) * 1000);
#endif
            struct linger l;
            l.l_onoff = 1;
            l.l_linger = 30;
            int INPROGRESS_ERR;
#ifdef _UNIX
            INPROGRESS_ERR = EINPROGRESS;
#else
            INPROGRESS_ERR = WSAEWOULDBLOCK;
#endif
            error = TcpIO.error();
            if (error && error != INPROGRESS_ERR) goto fail;
            ulStep++;
#ifdef _UNIX
            ::setsockopt(TcpIO.fd(), SOL_SOCKET, SO_LINGER,
                (const char*)&l, sizeof l);
#else
            ::setsockopt(TcpIO.socket(), SOL_SOCKET, SO_LINGER,
                (const char*)&l, sizeof l);
#endif
            error = TcpIO.error();
            if (error && error != INPROGRESS_ERR) goto fail;
            ulStep++;

            TcpIO.write("!!!!!!!!!!", 10);
#ifdef _UNIX
            sleep(ulFailures && ulFailures <= 3 ? 10 : 20);
#else
            Sleep((ulFailures && ulFailures <= 3 ? 10 : 20) * 1000);
#endif
            error = TcpIO.error();
            if (error && error != INPROGRESS_ERR) goto fail;
            ulStep++;

            n = TcpIO.read(buf, 1024);
            error = TcpIO.error();
            if (error && error != INPROGRESS_ERR) goto fail;
            ulStep++;

fail:
            // we need to be able to stop the alive checker at this
            // point in case we are shutting down the server
            if (g_bStopAliveChecker)
            {
                TcpIO.close();
                bDisable = TRUE;
                g_bStopAliveChecker = FALSE;
                continue;
            }

            if (n >= 3 && !strncasecmp(buf, "200", 3))
            {
                /* Good! */
                //printf ("*** Alive Checker Run Good!\n");
                if (g_bShowDebugErrorMessages)
                {
                    printf ("* Heartbeat Check OK\n");
                    fflush(0);
                }
                ulFailures = 0;
                bEverSucceeded = TRUE;
            }
            else
            {
                //printf ("*** Alive Checker Failed!\n");
                ulFailures++;
                printf("* Heartbeat Failure %d (Step %d) -- error(%d)\n", ulFailures, ulStep, error);
                if (ulFailures > 0 && ulFailures < 3)
                {
                    printf("\n-------------------------------------------------------------------------------\n");
                    printf("*** %s Heartbeat Failure Report\n",
                           ServerVersion::ProductName());
                    struct tm tm;

                    char buffer[4096];
                    HXTime now;
                    gettimeofday(&now, NULL);
                    hx_localtime_r((const time_t *)&now.tv_sec, &tm);
                    strftime(buffer, sizeof buffer, "When: %d-%b-%y %H:%M:%S\n", &tm);
                    printf("%s", buffer);
                    printf("Environment: %s, %s, %s\n",
                           ServerVersion::Platform(),
                           ServerVersion::BuildName(),
                           ServerVersion::VersionString());
                    fflush(0);

#ifdef _UNIX
                    kill(g_MonitorPID, SIGXFSZ);
                    sleep(10);
#elif defined WIN32
                    DumpThreadState();
#endif
                }
                else if (ulFailures == 3)
                {
                    if (bEverSucceeded)
                    {
#ifdef _UNIX
                        kill(g_MonitorPID, SIGXFSZ);
                        sleep(30);
#endif
                        char buffer[4096];
                        struct tm tm;
                        HXTime now;
                        gettimeofday(&now, 0);
                        hx_localtime_r((const time_t *)&now.tv_sec, &tm);
                        strftime(buffer, sizeof buffer, "%d-%b-%y %H:%M:%S", &tm);
                        printf("\n\n%s failure (%s)...  Restarting\n\n\n",
                                ServerVersion::ProductName(), buffer);
                        fflush(0);
#ifdef _UNIX
                        kill(g_MonitorPID, SIGUSR1);
#else
                        RestartServer();
#endif
                    }
                    else
                    {
#ifdef _UNIX
                        kill(g_MonitorPID, SIGXFSZ);
#endif
                        printf("\n\n%s not responding normally...\nHeartbeat check disabled\n",
                                ServerVersion::ProductName());
                    }
                }
            }
            TcpIO.close();
        }
        else
        {
            //printf ("*** Alive Checker Not Running: No Port\n");
        }
    }
}


#ifdef _UNIX
Timeval g_ITInterval;

void
ITimerAlarmHandler(int code)
{
    long lOldSec = g_pNow->tv_sec;
    *g_pNow += g_ITInterval;

    // Resync once per second
    if (lOldSec != g_pNow->tv_sec)
    {
        gettimeofday(g_pNow, NULL);
    }
}

void
StartITimer(UINT32 ms)
{
    // unblock SIGALRM so that the timer thread can use the itimer.
    sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &set, NULL);

    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))ITimerAlarmHandler;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval value;
    Timeval tInterval((int)0, (int)(ms*1000));

    value.it_interval = tInterval;
    value.it_value = tInterval;

    gettimeofday(g_pNow, NULL);
    setitimer(ITIMER_REAL, &value, NULL);

    // Set g_ITInterval to actual interval
    getitimer(ITIMER_REAL, &value);
    g_ITInterval.tv_sec = value.it_interval.tv_sec;
    g_ITInterval.tv_usec = value.it_interval.tv_usec;
}

void
StopITimer(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))SIG_DFL;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval value;
    memset(&value, 0, sizeof(value));
    setitimer(ITIMER_REAL, &value, NULL);

}
#endif

void ITimerDoNothing()
{
    *pTimerProcInitMutex = 0;
#ifdef _WIN32
    while(1) Sleep(60000);
#else
    while(1) pause();
#endif
}

void
UnixTimerInitCallback::func(Process* proc)
{
#if defined(_WIN32)
    ITimerDoNothing();
#else
    BOOL bUseITimer = FALSE;
    UINT32 uTimerRes = 0;
    if (!g_bDisableFastClock)
    {
        suprintf("Calibrating timers...\n");
        Timeval tNow, tStart, tElapsed, tDiff;
        for (uTimerRes = 10; uTimerRes < 20; uTimerRes++)
        {
            if (g_bShowDebugErrorMessages)
            {
                suprintf("    Testing %ums resolution: ", uTimerRes);
            }
            gettimeofday(&tStart, NULL);
            StartITimer(uTimerRes);
            do
            {
                pause();
                gettimeofday(&tNow, NULL);
                tElapsed = tNow - tStart;
            } while (tElapsed.tv_sec < 2);
            StopITimer();
            tDiff = (tNow > *g_pNow) ? (tNow - *g_pNow) : (*g_pNow - tNow);

            if (g_bShowDebugErrorMessages)
            {
                suprintf("%ld.%03ldms actual, %ldms drift\n",
                         g_ITInterval.tv_usec/1000, g_ITInterval.tv_usec%1000,
                         tDiff.tv_sec*1000 + tDiff.tv_usec/1000);
            }

            // If the ITIMER is accurate, *g_pNow should be equal to tNow,
            // plus or minus one interval.
            if (tDiff <= g_ITInterval)
            {
                bUseITimer = TRUE;
                break;
            }
        }
    }

    if (bUseITimer)
    {
        suprintf("Interval timer enabled (%ums resolution).\n", uTimerRes);
        StartITimer(uTimerRes);
        *g_bITimerAvailable = TRUE;
    }
    else
    {
        suprintf("Interval timer disabled.\n");
        *g_bITimerAvailable = FALSE;
    }

    fflush(0);
    ITimerDoNothing();
    /* NOTREACHED */
#endif
}

MemCache** g_pMemCacheList = 0;
#if defined(MEMCACHE_DUMP)
#if defined(_UNIX)
void
MemCacheSigHandler(int code)
{
    MemCache::CacheDump();
}
#endif
#endif

#ifdef _LINUX
static BOOL
RedHatEtcHostsOkay()
{
    const UINT32 LocalHostIp = 0x0100007F;  // better known as 127.0.0.1
    char szHostName[2048];
    UINT32 interface_ip;

    if (0 == ::gethostname(szHostName,2048))
    {
        struct hostent* pHostent = ::gethostbyname(szHostName);
        if (pHostent != NULL)
        {
            interface_ip = *((u_long32*)(pHostent->h_addr));
            if (interface_ip == LocalHostIp)
            {
                return FALSE;
            }
            else
            {
                // we have a host ip addr that's not localhost
                return TRUE;
            }
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}
#endif

#ifdef _LINUX
void
exec_as_resolver()
{
    if ( g_bDisableInternalResolver )
    {
        execlp(restartargv[0], restartargv[0], "--resolver", NULL);
    }
    else
    {
        execlp(restartargv[0], restartargv[0], "--iresolver", NULL);
    }
}
#endif


// XXXBWV Hack: Because we need the startup log
// to contain all the options and things,
// we have to parse the commandline arguments
// for the config file, and manually parse the config
// file for the Logs directory which we will assume
// for now is the directory that the pid file is stored
// in (<Var PidPath="....."/>).  This code will have to
// be revisited if PidPath is changed, or if we want
// to and a config value for explicitly specifying the
// "Logs/" directory.  If PidPath isn't there, it
// default to the current directory.
#define LOG_FILE_VAR "PidPath"

#if _WIN32
#define SLASH '\\'
#else
#define SLASH '/'
#endif

void
GetStartupLogFilename(int argc, char **argv)
{
    // First look for the config file in the
    // commandline arguments...
    int i = 1;
    bool bIgnoreNext = false;
    char *szConfigFile = NULL;

    while (i < argc)
    {
        if (!bIgnoreNext && argv[i][0] != '-')
        {
            szConfigFile = new char[strlen(argv[i])+1];
            strcpy(szConfigFile, argv[i]);
            break;
        }

        bIgnoreNext = false;

        if (argv[i][0] == '-')
        {
            bIgnoreNext = true;
        }
        i++;
    }

    // If we didn't find the config file, don't
    // worry about the startup log path because
    // startup will fail without a config file.
    if (szConfigFile == NULL)
        return;

    FILE *fp = fopen(szConfigFile, "r");

    delete szConfigFile;

    // Again, just bail if the file doesn't open...
    if (fp == NULL)
        return;

    // How the next bit works:
    //  It fills a buffer and reads one character at a time from it.
    //  First it looks for '<', then 0 or more spaces, then "Var",
    //  then 1 or more spaces, then "PidPath", then 0 or more spaces,
    //  then '=', then 0 or more spaces, then '\"', then it copies
    //  all characters until it hits another '\"'.  If it runs out
    //  of buffer, it reads some more.  If at anytime it hits a
    //  problem (like eof), it returns without error because someone
    //  else will complain if the config is bogus.  It restarts
    //  (state = 0) if an attempt to match fails.  If it sees "!--"
    //  after a '<', it ignores all characters until it sees "-->".

    char *ptr = NULL;
    char buffer[2048];
    int len;
    int state = 0;
    // Important strings we might see...
    const char szStrVar[] = "Var";
    const char szStrPidPath[] = LOG_FILE_VAR;
    const char szStrComment[] = "!--";
    const char szStrEndComment[] = "-->";

    i = 0;
    // One loop for each char...
    for (;;)
    {
        // fill buffer if needed
        if (ptr == NULL || *ptr == '\0')
        {
            len = fread(buffer, 1, (sizeof buffer)-1, fp);
            if (len == 0)
                goto use_default;
            buffer[len] = '\0';
            ptr = &(buffer[0]);
        }

        switch (state)
        {
        // Look for '<'
        case 0:
            if (*ptr == '<')
                state++;
            break;
        // Skip any white space
        case 1:
            // Beware of "<!--"'s...
            if (i == (int)strlen(szStrComment))
            {
                i = 0;
                state = 11;
                goto advance;
            }
            if (*ptr == szStrComment[i])
            {
                i++;
                goto advance;
            }
        case 4:
        case 6:
        case 8:
            if (!isspace(*ptr))
                goto next_no_advance;
            break;
        // Look for "Var"
        case 2:
            if (i == (int)strlen(szStrVar))
                goto next_no_advance;
            if (*ptr == szStrVar[i])
                i++;
            else
                goto start_over;
            break;
        // Look for a space
        case 3:
            if (!isspace(*ptr))
                state = 0;
            else
                state++;
            break;
        // Look for "PidPath"
        case 5:
            if (i == (int)strlen(szStrPidPath))
                goto next_no_advance;
            if (*ptr == szStrPidPath[i])
                i++;
            else
                goto start_over;
            break;
        // Look for '='
        case 7:
            if (*ptr == '=')
                state++;
            else
                goto start_over;
            break;
        // Look for '\"'
        case 9:
            if (*ptr == '\"')
                state++;
            else
                goto start_over;
            break;
        // Read until '\"'...
        case 10:
            if (i > MAX_STARTUPLOG_LEN)
                goto use_default;
            if (*ptr == '\"')
            {
                g_szStartupLog[i] = '\0';
                goto strip_filename;
            }
            g_szStartupLog[i] = *ptr;
            i++;
            break;
        // Read all chars until we hit "-->"
        case 11:
            if (i == (int)strlen(szStrEndComment))
                goto start_over;
            if (*ptr == szStrEndComment[i])
                i++;
            break;
        }
advance:
        ptr++;
        continue;
start_over:
        i = 0;
        state = 0;
        continue;
next_no_advance:
        i = 0;
        state++;
        continue;
    }

strip_filename:

    ptr = strrchr(g_szStartupLog, SLASH);

    // If there aren't any slashes, just put the filename in...
    if (ptr == NULL)
    {
        strcpy(g_szStartupLog, STARTUP_LOG);
    }
    else
    {
        // truncate the Pid filename, put the startup filename on...
        *(ptr+1) = '\0';
        strcat(g_szStartupLog, STARTUP_LOG);
    }

    g_bUseStartupLog = TRUE;

    return;

use_default:

    strcpy(g_szStartupLog, STARTUP_LOG);
    g_bUseStartupLog = TRUE;
    return;
}

void
PrintVersion()
{
    // Adding Time information to "startup.log"
    struct tm tm;
    HXTime now;
    char timeStamp[80];
    gettimeofday(&now, NULL);
    hx_localtime_r((const time_t *)&now.tv_sec, &tm);
    if (strftime(timeStamp, 80, "%d-%b-%y %H:%M:%S\n", &tm) > 0)
    {
        suprintf("\nServer Started:   %s", timeStamp);
    }

    suprintf("%s %s\n", ServerVersion::ProductName(), HX_COPYRIGHT_INFO);

    suprintf("Version:   %s ", ServerVersion::ProductName());
    const char* szRelName = ServerVersion::ReleaseName();
    if (strlen(szRelName))
    {
        suprintf("%s ", szRelName);
    }
    else
    {
        suprintf("%s ", ServerVersion::MajorMinorString());
    }
    suprintf("(%s)", ServerVersion::VersionString());
    const char* szBldName = ServerVersion::BuildName();
    if (strlen(szBldName))
    {
        suprintf(" %s", szBldName);
    }
#ifdef _DEBUG
    suprintf(" [DEBUG]");
#endif
#ifdef _STATICALLY_LINKED
    suprintf(" [_STATICALLY_LINKED]");
#endif
    suprintf("\n");

    suprintf("Platform:  %s\n", ServerVersion::Platform());

    suprintf("\n");
    fflush(0);
}

void
PrintVersionVerbose()
{
    suprintf("Product:   %s %s\n", ServerVersion::ProductName(),
                                 ServerVersion::ReleaseName());
    suprintf("Build ID:  %d\n", ServerVersion::BuildID());
    suprintf("Timestamp: %s\n", ServerVersion::BuildTimestamp());
    suprintf("Tag:       %s\n", ServerVersion::BuildTag());
    suprintf("Tag ID:    %d\n", ServerVersion::BuildTagID());
    suprintf("Branch:    %s\n", ServerVersion::Branch());
    suprintf("Platform:  %s\n", ServerVersion::Platform());
}

void
PrintUsage()
{

// some flags we allow but don't wish to advertise to the end user:
#ifdef DEBUG
#define XXX_INTERNAL_FLAGS 1
#endif

    fprintf (stderr, "Usage: %s [options] configuration-file\n", progname);

    fprintf (stderr,

// GENERAL OPTIONS
"\nWhere options are:\n"
"-v          --version                   Print version number and exit\n"
"-V          --Version                   Print verbose version info and exit\n"
"-h          --help                      Print command-line help and exit\n"
"--out <fn>  --output-file <file>        Redirect console output to file\n"
"--debug     (shorthand for --cp --acd --sdm --rss --mca)\n"
"--rss [n]   --report-server-stats [n]   Report Every n seconds (default 60s)\n"
"--cp        --crash-avoidance-print     Print info about avoided crashes\n"
"--nca       --no-crash-avoidance        Disable crash-avoidance\n"
"--mca [n]   --max-crash-avoidance [n]   Maximum CAs before server restarts\n"
"--acd       --allow-core-dump           Allow creation of coredump files\n"
"--nar       --no-auto-restart           Don't restart server after fatal crashes\n"
"--sct       --skip-cpu-test             Use config file CPU setting\n"
"--sdm       --show-debug-messages       Show additional runtime messages\n"
"--dhb       --disable-heart-beat        Disable Heartbeat check\n"
"--hbi <IP>  --heart-beat-ip <IP>        Check for server heartbeat on this IP\n"
"-m <size>   --memory <size>             Use 'size' memory only (in MB)\n"
#ifdef XXX_INTERNAL_FLAGS
"--ftp       --force-threadsafe-plugins  Treat all plugins as if threadsafe\n"
"--msm       --mem-size-map\n"
"--dpa       --disable-packet-aggregation\n"
"--dfc       --disable-fast-clock\n"
"--ssa       --slight-send-acceleration\n"
"--aco <n>   --alloc-coefficient <n>     Coefficient for heap polynomial\n"
"--nfm       --no-fast-malloc            Disable per-thread memory cache\n"
#if FAST_MALLOC_ALL
"--lfm       --limited-fast-malloc       Use less memory caching\n"
#else
"--fma       --fast-malloc-all           Use extensive memory caching\n"
#endif
#ifdef DEV_POLL_SUPPORT
"--ndp       --no-dev-poll               Disable use of /dev/poll\n"
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
"--nep       --no-epoll                  Disable use of Linux epoll\n"
#endif // LINUX_EPOLL_SUPPORT
#ifdef PTHREADS_SUPPORTED
"--uth       --use-threads               Use threads instead of processes\n"
"--upr       --use-processes             Use processes instead of threads\n"
#endif // PTHREADS_SUPPORTED
#endif //XXX_INTERNAL_FLAGS

// UNIX OPTIONS
#ifdef _UNIX
"Unix-specific options:\n"
"--dir       --disable-internal-resolver Use system hostname resolver\n"
"--bop <n>   --back-off-percentage <n>   Back-off percentage for initial mmap()\n"
#endif

// LINUX OPTIONS
#ifdef _LINUX
"Linux-specific options:\n"
#ifdef XXX_INTERNAL_FLAGS
"--nss       --no-segv-syscall\n"
"--gdb                                   Work properly with gdb\n"
"--sdt       --shared-descriptor-tables  Share Descriptor Tables among threads\n"
"--unsupported-os                        Attempt to run even if unsupported/incompatable OS\n"
#endif //XXX_INTERNAL_FLAGS
#endif


// WINDOWS OPTIONS
#ifdef _WIN32
"Windows-specific options:\n"
"-import[:Key] <file>                    Import the configuration-file into\n"
"                                        the Windows Registry\n"
"registry:<Key>                          Use the configuration stored in\n"
"                                        the Windows Registry\n"
"-install[:Name] \"Params\"                Install the server as a Windows NT\n"
"                                        Service called \"Name\", passing\n"
"                                        it the parameter list \"Params\"\n"
"                                        upon startup\n"
"-remove[:Name]                          Remove the Windows NT Service \n"
"                                        called \"Name\"\n"
"-N                                      Run as an NT service\n"
#endif

// DEBUG OPTIONS
#ifdef DEBUG
"Debug options:\n"
#ifdef XXX_INTERNAL_FLAGS
"--ncv        --no-client-validation\n"
#endif
#ifdef PAULM_LEAKCHECK
"--lct <n:n:n> --leak-check-test <n:n:n> Leak check <FirstWait>:<Alloc>:<Free>\n"
#endif
"-d <mask>                               Enable debugging messages per mask\n"
"   multiple bits may be ORed together. They are:\n"
"      0x00000001     print error\n"
"      0x00000002     print informative messages\n"
"      0x00000004     print procedure entry/exit messages\n"
"      0x00000008     print state machine messages\n"
"      0x00000010     print read/write messages\n"
"      0x00000020     print timing related messages\n"
"      0x00000040     print file offsets\n"
"      0x00000080     print statistics\n"
"      0x00000100     print select call returned\n"
"      0x00000200     print file descriptor accounting\n"
"      0x00000400     print protocol info\n"
"      0x00000800     print allocation info\n"
"      0x00001000     print accounting messages\n"
"      0x00002000     print licensing messages\n"
"      0x00004000     print profiling information\n"
"      0x00008000     print misc. other messages \n"
"      0x00010000     print registry messages\n"
"-D <mask>                               Enable debugging functions per mask\n"
"   multiple bits may be ORed together. They are:\n"
"      0x01           server does not respond to challenges\n"
"      0x02           server sends a bad challenge\n"
"      0x04           server drops every 10th UDP packet\n"
"      0x08           server turns resend off\n"
#ifdef _WIN32
"-P                                      Enable Memory leak tracking (NT only)\n"
#endif
#endif

#if defined PAULM_ALLOCTRACK && defined _UNIX
"-O           --ogre                     Enable OgreDebug\n"
#endif
#ifdef PAULM_ALLOCDUMP
"--ad         --alloc-dumping            Debug - Allocation dumping\n"
#endif
#if defined PAULM_REPORT
"--st         --socket-timing            Debug - socket timing\n"
#endif
#ifdef _EFENCE_ALLOC
"--bfa        --back-fence-alloc         Debug - _EFENCE_ALLOC before memory\n"
"--ea [n]     --efence-alloc [n]         Debug - use _EFENCE_ALLOC [bucket n only]\n"
"--pfm        --protect-freed-memory     Debug - mprotect freed memory for blocks\n"
"                                                that take at least a whole page\n"
#endif

    );
}

#ifdef  MSDOS
#define SEP     '\\'
#else
#define SEP     '/'
#endif

const char*
GetProgName(const char* arg)
{
    const char* progname = strrchr(arg, SEP);
    return progname ? progname + 1 : arg;
}

/***********************************************************************
 * _main()
 */
int
_main(int argc, char** argv)
{
    int i = 0;
    char** arg = 0;
    char* pszTmpConfigFile = 0;
    char* pszTmpImportKey = 0;

    // force several command-line options:
    g_bPrintProcessPIDs = TRUE;

#ifndef _WIN32
    // These flags are used when we kick off resolver prccesses
    // and are not intended to be used by the end-user.
    if (argc == 2 && !strcmp(argv[1], "--resolver"))
    {
        // don't let resolver core dump
        struct rlimit r = {0, 0};
        setrlimit(RLIMIT_CORE, &r);

        ResolverMUX::do_hostnames(fileno(stdin), fileno(stdout));
        return -1;
    }
    if (argc == 2 && !strcmp(argv[1], "--iresolver"))
    {
        // don't let resolver core dump
        struct rlimit r = {0, 0};
        setrlimit(RLIMIT_CORE, &r);

        ResolverMUX::do_hostnames_internal(fileno(stdin), fileno(stdout));
        return -1;
    }
#endif

    // XXXBWV: Parse the commandline, find the config
    // file, dig through it to find the Logs directory
    // path, use that as the location for the startup log
    GetStartupLogFilename(argc, argv);

    progname = GetProgName(argv[0]);

    PrintVersion();

    // First, scan all args for -v, -V or -h; if there we want to exit now...
    for (i = argc, arg = argv; i; arg++, i--)
    {
        if (!strcmp(*arg, "--version") ||
            !strcmp(*arg, "-v"))
        {
            // already printed it above so we're done
            exit(0);
        }
        if (!strcmp(*arg, "--Version") ||
            !strcmp(*arg, "-V"))
        {
            PrintVersionVerbose();
            exit(0);
        }
        if (!strcmp(*arg, "--help") ||
            !strcmp(*arg, "-h") ||
            !strcmp(*arg, "/?")) // for win32-heads
        {
            PrintUsage();
            exit(0);
        }
    }

    RSSManager::m_ulRSSReportInterval = 0;
    RSSManager::m_bRSSReportEnabled = FALSE;

    // now scan all args for --out so we can redirect stdout/stderr first thing
    for (i = argc, arg = argv; i; arg++, i--)
    {
#ifdef _WIN32
        if (!strcmp(*arg, "--nt-stdout-file") || //obsolete
            !strcmp(*arg, "--nsf") ||            //obsolete
            !strcmp(*arg, "--output-file") ||
            !strcmp(*arg, "--out"))
#else
        if (!strcmp(*arg, "--output-file") ||
            !strcmp(*arg, "--out"))
#endif
        {
            if (i > 1 && *(arg+1)[0] != '-')
            {
                i--;
                arg++;
                suprintf("Option: redirecting output to %s\n", *arg);

                //make stdout/stderr line-buffered as well as redirecting them
                FILE* fp1 = freopen(*arg, "a", stdout);
                FILE* fp2 = freopen(*arg, "a", stderr);
                char* pBuf1 = new char[4096];
                char* pBuf2 = new char[4096];
                setvbuf(stdout, pBuf1, _IOLBF, 4096);
                setvbuf(stderr, pBuf2, _IOLBF, 4096);
                if (!fp1)
                {
                    suprintf("Error: failed to redirect stdout to %s\n", *arg);
                }
                if (!fp2)
                {
                    suprintf("Error: failed to redirect stderr to %s\n", *arg);
                }
                if (fp1 && fp2)
                {
                    suprintf("\n");
                    PrintVersion();  //print this again so it's in the log
                    suprintf("Option: redirecting output to %s\n", *arg);
                }
            }
            else
            {
                suprintf("Option: %s must take a filename!\n", *arg);
            }

            break;
        }
    }

    /*
     * We can't use Options because it's too late.
     */
    for (i = argc, arg = argv; i; arg++, i--)
    {
        if (arg == argv)
        {
            // this is the command name, so skip it
        }

#ifdef _WIN32
        else if (!strcmp(*arg, "--nt-stdout-file") ||
                 !strcmp(*arg, "--nsf"))
        {
            // for WIN32 we already redirected stdout above
            suprintf("Warning: %s is obsolete, use --out <file>\n", *arg);
            if (i > 1 && *(arg+1)[0] != '-')
            {
                i--;
                arg++;
            }
        }
#endif
        else if (!strcmp(*arg, "--output-file") ||
                 !strcmp(*arg, "--out"))
        {
            // we already redirected stdout above
            if (i > 1 && *(arg+1)[0] != '-')
            {
                i--;
                arg++;
            }
        }
        // set maximum stack size
       else if (!strcmp(*arg, "--set-stack-size") ||
                !strcmp(*arg, "--sss"))
       {
#if defined _LINUX || defined PTHREADS_SUPPORTED
           suprintf("Option: Set Stack Size ");
           if (i > 1 && **(arg+1) >= '0' && **(arg+1) <= '9')
           {
               arg++;
               i--;
               g_ulThreadStackSize = atoi(*arg) * 1024;
           }
           else
           {
               g_ulThreadStackSize = 1024 * 1024;
           }

            suprintf("%dK\n", g_ulThreadStackSize / 1024);
#else
            suprintf("%s is not available on this platform (ignoring)\n", *arg);
           if (i > 1 && **(arg+1) >= '0' && **(arg+1) <= '9')
           {
               arg++;
               i--;
           }
#endif // defined _LINUX || defined PTHREADS_SUPPORTED
       }

        // limit CAs
        else if (!strcmp(*arg, "--max-crash-avoidance") ||
                 !strcmp(*arg, "--mca"))
        {
            // we printf this option later so it only prints once if --debug
            // was also used
            if (i > 1 && **(arg+1) >= '0' && **(arg+1) <= '9')
            {
                arg++;
                i--;
                g_ulMaxCrashAvoids = atoi(*arg);
            }
            else
            {
                g_ulMaxCrashAvoids = 1000;
            }
        }
        else if (!strcmp(*arg, "--no-crash-avoidance") ||
                 !strcmp(*arg, "--nca"))
        {
            suprintf("Option: Crash avoidance disabled\n");
            g_bNoCrashAvoidance = TRUE;
        }

        else if (!strcmp(*arg, "--crash-avoidance-print") ||
                 !strcmp(*arg, "--cp"))
        {
            suprintf("Option: Crash avoidance output enabled\n");
            g_bCrashAvoidancePrint = TRUE;
        }

        else if (!strcmp(*arg, "--print-process-pids") ||
                 !strcmp(*arg, "--ppp"))
        {
            suprintf("Option: Print Process IDs (obsolete, always true)\n");
        }

        else if (!strcmp(*arg, "--rro"))
        {
            suprintf("Option: Report Restart\n");
            g_bReportRestart = TRUE;
        }

        else if (!strcmp(*arg, "--no-auto-restart") ||
                 !strcmp(*arg, "--nar"))
        {
            suprintf("Option: No Auto Restart\n");
            g_bNoAutoRestart = 1;
        }

#ifdef DEBUG
        else if (!strcmp(*arg, "--no-client-validation") ||
                 !strcmp(*arg, "--ncv"))
        {
            suprintf("Option: Client validation disabled\n");
            g_bNoClientValidation = TRUE;
        }
#endif

        else if (!strcmp(*arg, "--mem-size-map") ||
                 !strcmp(*arg, "--msm"))
        {
#ifdef _UNIX
            suprintf("Option: Memory Allocation Size Map\n");
            g_bDoSizes = TRUE;
#else
            suprintf("%s is only available on Unix (ignoring)\n", *arg);
#endif
        }
        else if (!strcmp(*arg, "--no-fast-malloc") ||
                 !strcmp(*arg, "--nfm"))
        {
            suprintf("Option: No Fast Malloc\n");
            g_bFastMalloc = FALSE;
            g_bFastMallocAll = FALSE;
        }
        else if (!strcmp(*arg, "--limited-fast-malloc") ||
                 !strcmp(*arg, "--lfm"))
        {
#if FAST_MALLOC_ALL
            suprintf("Option: Limited Fast Malloc\n");
            g_bFastMalloc = TRUE;
            g_bFastMallocAll = FALSE;
#else
            suprintf("Option: Limited Fast Malloc (default)\n");
#endif
        }
        else if (!strcmp(*arg, "--fast-malloc-all") ||
                 !strcmp(*arg, "--fma"))
        {
#if FAST_MALLOC_ALL
            suprintf("Option: Fast Malloc All (default)\n");
#else
            suprintf("Option: Fast Malloc All\n");
            g_bFastMalloc = TRUE;
            g_bFastMallocAll = TRUE;
#endif
        }
        else if (!strcmp(*arg, "--report-server-stats") ||
                 !strcmp(*arg, "--rss"))
        {
            RSSManager::m_ulRSSReportInterval = 0;
            RSSManager::m_bRSSReportEnabled = TRUE;
            if (i > 1)
            {
                char* str = (*arg)+2;
                if (*str != ' ' && *str != '\t' && *str >= '0' && *str <= '9')
                {
                    arg++;
                    i--;
                    RSSManager::m_ulRSSReportInterval = atoi(str);
                }
                else if (*(arg+1) && **(arg+1) >= '0' && **(arg+1) <= '9')
                {
                    arg++;
                    i--;
                    RSSManager::m_ulRSSReportInterval = atoi(*arg);
                }
            }
            else
            {
                char* str = (*arg)+2;
                // printf("str = %s\n", str);
                if (*str != ' ' && *str != '\t' && *str >= '0' && *str <= '9')
                {
                    RSSManager::m_ulRSSReportInterval = atoi(str);
                }
            }
        }
        else if (!strcmp(*arg, "--report-dll-entry") ||
                 !strcmp(*arg, "--rde"))
        {
            suprintf("Option: Report DLL Entry Points (obsolete)\n");
            g_bReportHXCreateInstancePointer = TRUE;
        }
        else if (!strcmp(*arg, "--force-threadsafe-plugins") ||
                 !strcmp(*arg, "--ftp"))
        {
            suprintf("Option: Force ThreadSafe Plugins\n");
            g_bForceThreadSafePlugins = TRUE;
        }
        else if (!strcmp(*arg, "--skip-cpu-test") ||
                 !strcmp(*arg, "--sct"))
        {
            suprintf("Option: Skip CPU Test\n");
            g_bSkipCPUTest = TRUE;
        }
        else if (!strcmp(*arg, "--allow-core-dump") ||
                 !strcmp(*arg, "--acd"))
        {
            suprintf("Option: Allow Core Dump\n");
            g_bAllowCoreDump = TRUE;
        }
        else if (!strcmp(*arg, "--show-debug-messages") ||
                 !strcmp(*arg, "--sdm"))
        {
            suprintf("Option: Show Debug Messages\n");
            g_bShowDebugErrorMessages = TRUE;
        }
        else if (!strcmp(*arg, "--disable-heart-beat") ||
                 !strcmp(*arg, "--dhb"))
        {
            suprintf("Option: Disable HeartBeat\n");
            g_bDisableHeartbeat = TRUE;
        }
        else if (!strcmp(*arg, "--heart-beat-ip") ||
                 !strcmp(*arg, "--hbi"))
        {
            if (i && *(arg+1)[0] != '-')
            {
                arg++;
                i--;
                g_pHeartBeatIP = new_string(*arg);
            }
            suprintf("Option: HeartBeat IP: %s\n", g_pHeartBeatIP);
        }
        else if (!strcmp(*arg, "--disable-packet-aggregation") ||
                 !strcmp(*arg, "--dpa"))
        {
            suprintf("Option: Disable Packet Aggregation\n");
            g_bDisablePacketAggregation = TRUE;
        }
        else if (!strcmp(*arg, "--disable-fast-clock") ||
                 !strcmp(*arg, "--dfc"))
        {
            suprintf("Option: Disable Fast Clock\n");
            g_bDisableFastClock = TRUE;
        }
        else if (!strcmp(*arg, "--alloc-coefficient") ||
                 !strcmp(*arg, "--aco"))
        {
            arg++;
            i--;
            g_AllocCoefficient = (float)atof(*arg);
            suprintf("Option: Allocator Coefficient: %f\n", g_AllocCoefficient);
        }
        else if (!strcmp(*arg, "--no-segv-syscall") ||
                 !strcmp(*arg, "--nss"))
        {
#ifdef _LINUX
            suprintf("Option: Don't use segv syscall\n");
            g_ulLinuxNoSegvSyscall = 1;
#else
            suprintf("%s is only available on Linux (ignoring)\n", *arg);
#endif
        }
#ifdef _EFENCE_ALLOC
        else if (!strcmp(*arg, "--protect-freed-memory") ||
                 !strcmp(*arg, "--pfm"))
        {
            suprintf("Option: EFENCE protect freed memory\n");
            g_bEFenceProtectFreedMemory = TRUE;
        }
        else if (!strcmp(*arg, "--back-fence-alloc") ||
                 !strcmp(*arg, "--bfa"))
        {
            suprintf("Option: EFENCE before pages\n");
            g_backfence_alloc = 1;
        }
        else if (!strcmp(*arg, "--efence-alloc") ||
                 !strcmp(*arg, "--ea"))
        {
            if (i > 1 && **(arg+1) >= '0' && **(arg+1) <= '9')
            {
                arg++;
                i--;
                g_efence_alloc = atoi(*arg);
                suprintf("Option: _EFENCE_ALLOC bucket %d\n", g_efence_alloc);
            }
            else
            {
                g_efence_alloc = -1;
                suprintf("Option: _EFENCE_ALLOC all buckets\n");
            }

        }
#endif
        else if (!strcmp(*arg, "--debug"))
        {
            suprintf("Option: Crash avoidance output enabled\n");
            g_bCrashAvoidancePrint = TRUE;
            suprintf("Option: Allow Core Dump\n");
            g_bAllowCoreDump = TRUE;
            suprintf("Option: Show Debug Messages\n");
            g_bShowDebugErrorMessages = TRUE;
            RSSManager::m_bRSSReportEnabled = TRUE;
            g_ulMaxCrashAvoids = 1000;
        }
        else if (!strcmp(*arg, "--slight-send-acceleration") ||
                 !strcmp(*arg, "--ssa"))
        {
            suprintf("Option: Slight send acceleration enabled\n");
            g_bSlightAcceleration = TRUE;
        }
        else if (!strcmp(*arg, "--disable-internal-resolver") ||
                 !strcmp(*arg, "--dir"))
        {
#ifdef _UNIX
            suprintf("Option: Disable Internal Resolver\n");
            g_bDisableInternalResolver = TRUE;
#else
            suprintf("%s is only available on Unix (ignoring)\n", *arg);
#endif
        }
#ifdef _UNIX
#if defined PAULM_REPORT
        else if (!strcmp(*arg, "--socket-timing") ||
                 !strcmp(*arg, "--st"))
        {
            suprintf("Socket timing enabled. TTIN to check.\n");
        }
#endif
#ifdef PAULM_ALLOCDUMP
        else if (!strcmp(*arg, "--alloc-dumping") ||
                 !strcmp(*arg, "--ad"))
        {
            suprintf("Allocation dumping...\n");
            m_bAllocDump = 1;
        }
#endif
#endif

#ifdef PAULM_LEAKCHECK
        else if (!strcmp(*arg, "--leak-check-test") ||
                 !strcmp(*arg, "--lct"))
        {
            arg++;
            i--;
            if (!i || !strchr(*arg, ':'))
            {
                suprintf("--lct needs <FirstWait>:<Alloc>:<Free>\n");
                exit(-1);
            }
            sscanf(*arg, "%d:%d:%d", &g_ulLeakCheckFirst,
                    &g_ulLeakCheckAlloc, &g_ulLeakCheckFree);
            suprintf("Option --lct:\n");
            suprintf("\tFirst: %d\n", g_ulLeakCheckFirst);
            suprintf("\tAlloc: %d\n", g_ulLeakCheckAlloc);
            suprintf("\tFree:  %d\n", g_ulLeakCheckFree);
            g_bLeakCheck = TRUE;
        }
#endif

#ifdef PAULM_ALLOCTRACK
        else if (!strcmp(*arg, "--ogre") ||
                 !strcmp(*arg, "-O"))
        {
            g_bOgreDebug = TRUE;
            suprintf("Option: OgreDebug\n");
        }
#endif //PAULM_ALLOCTRACK

        else if (!strncmp(*arg, "-import", 7))
        {
#ifdef _WIN32
            if (strlen(*arg) > 8)
            {
                if ((*arg)[7] == ':')
                {
                    pszTmpImportKey = new_string(*arg+8);
                }
            }
            if (!pszTmpImportKey)
            {
                pszTmpImportKey = new_string("Config");
            }
            suprintf("Option: Import into registry key %s\n", pszTmpImportKey);
#else
            suprintf("Error: -import is only available on NT (ignoring)\n");
#endif
        }
        else if (!strcmp(*arg, "--sdt") ||
                 !strcmp(*arg, "--share-descriptor-tables"))
        {
#ifdef _LINUX
            suprintf("Option: Share Descriptor Tables\n");
            g_bSharedDescriptors = 1;
#else
            suprintf("%s is not available on this platform (ignoring)\n", *arg);
#endif /* _LINUX */
        }
        else if (!strcmp(*arg, "--uth") || !strcmp(*arg, "--use-threads"))
        {
#ifdef PTHREADS_SUPPORTED
            suprintf("Option: Use Threads\n");
            g_bUnixThreads = 1;
            g_bSharedDescriptors = 1;
#else
            suprintf("%s is not available on this platform (ignoring)\n", *arg);
#endif // PTHREADS_SUPPORTED
        }
        else if (!strcmp(*arg, "--upr") || !strcmp(*arg, "--use-processes"))
        {
#ifdef PTHREADS_SUPPORTED
            suprintf("Option: Use Processes\n");
            g_bUnixThreads = 0;
            g_bSharedDescriptors = 0;
#else
            suprintf("%s is not available on this platform (ignoring)\n", *arg);
#endif // PTHREADS_SUPPORTED
        }
        else if (!strcmp(*arg, "--ndp") || !strcmp(*arg, "--no-dev-poll"))
        {
#ifdef DEV_POLL_SUPPORT
            suprintf("Option: Don't Use Dev Poll for Select\n");
            g_bUseDevPoll = 0;
#else
            suprintf("%s is not available on this platform (ignoring)\n", *arg);
#endif // DEV_POLL_SUPPORT
        }

        else if (!strcmp(*arg, "--nep") || !strcmp(*arg, "--no-epoll"))
        {
#ifdef LINUX_EPOLL_SUPPORT
            suprintf("Option: Don't Use EPoll for Select\n");
            g_bUseEPoll = 0;
#else
            suprintf("%s is not available on this platform (ignoring)\n", *arg);
#endif // DEV_POLL_SUPPORT
        }

#ifdef _DEBUG
        else if (!strncmp(*arg, "-d", 2))
        {
            if (strlen(*arg) != 2) //support -dN
            {
                char* endptr = *arg+2+strlen(*arg+2);
                g_ulDebugFlags = strtoul(*arg+2, &endptr, 16); // hex argument
            }
            else
            {
                arg++;
                i--;
                char* endptr = *arg+strlen(*arg);
                g_ulDebugFlags = strtoul(*arg, &endptr, 16);
            }
            suprintf("Option: Debug Flags: 0x%x\n", g_ulDebugFlags);
        }
        else if (!strncmp(*arg, "-D", 2))
        {
            if (strlen(*arg) != 2) //support -DN
            {
                char* endptr = *arg+2+strlen(*arg+2);
                g_ulDebugFuncFlags = strtoul(*arg+2, &endptr, 16); // hex arg
            }
            else
            {
                arg++;
                i--;
                char* endptr = *arg+strlen(*arg);
                g_ulDebugFuncFlags = strtoul(*arg, &endptr, 16);
            }
            suprintf("Option: Debug Function Flags: 0x%x\n", g_ulDebugFuncFlags);
        }
#endif //_DEBUG

        else if (!strcmp(*arg, "--bop") ||
                 !strcmp(*arg, "--back-off-percentage"))
        {
            suprintf("Option: mmap() Back-Off Percentage\n");
            if (i > 1 && **(arg+1) >= '0' && **(arg+1) <= '9')
            {
                arg++;
                i--;
                g_ulBackOffPercentage = atoi(*arg);
                if (!g_ulBackOffPercentage)
                    g_ulBackOffPercentage = 10;
                else if (g_ulBackOffPercentage > 50)
                    g_ulBackOffPercentage = 50;
            }
        }

        else if (!strncmp(*arg, "-m", 2) || !strcmp(*arg, "--memory"))
        {
            UINT32 ulSize = 0;
            if (strlen(*arg) != 2 && *(*arg+1) != '-') //support -mN
            {
                ulSize = atoi(*arg+2);
            }
            else if (i > 1)
            {
                arg++;
                i--;
                ulSize = atoi(*arg);
            }

            suprintf("Option: Max Server Mem Size %luMB\n", ulSize);
            // min mem size should match what's in server/engine/core/shregion.cpp
            if (ulSize >= MIN_SHMEM_SIZE)
            {
#if !defined(_LONG_IS_64)
                if (ulSize > 4095)
                {
                    // Since this value gets multiplied n * 1024 * 1024 there
                    // is a maximum limit of 4095 on 32-bit OSes.
                    suprintf("Warning: Invalid Mem Size %luMB (maximum 4095). "
                    "Resetting to 4095MB\n", ulSize);
                    ulSize = 4095;
                }
#endif
                g_ulSizemmap = ulSize;
            }
            else
            {
                // min mmap size should b MIN_SHMEM_SIZE MB, so if the -m argument is < MIN_SHMEM_SIZE
                // then reset it to at least MIN_SHMEM_SIZE, since it can b assumed that
                // the user wanted the server to use as little memory as
                // possible
                suprintf("Warning: Invalid Mem Size %luMB (minimum %d). "
                    "Resetting to %dMB\n", ulSize, MIN_SHMEM_SIZE, MIN_SHMEM_SIZE);
                g_ulSizemmap = MIN_SHMEM_SIZE;
            }
        }
        else if (!strcmp(*arg, "--unsupported-os"))
        {
            suprintf("Option: Unsupported OS\n");
            g_bUnsupportedOSFlag = TRUE;
        }


        //XXXDC insert any new options before this (and document them...=)
        else if (*arg[0] == '-')
        {
            suprintf("Unknown option: %s\n", *arg);
            PrintUsage();
            terminate(1);
        }
        else
        {
            if (!pszTmpConfigFile)
            {
                pszTmpConfigFile = new_string(*arg);
                // will log below, after printing all the options
            }
            else
            {
                suprintf("Warning: Multiple config files listed on command line,"
                        " ignoring %s\n", *arg);
            }
        }
    }

    if (g_bLeakCheck || g_bOgreDebug)
    {
        //Someday we'll make the leak-checker MemCache-aware, until then
        //we need to disable it or we'll see a lot of false leaks.
        if (g_bFastMalloc == TRUE)
        {
            suprintf("Option: No Fast Malloc (--nfm option forced for "
                    "leak-checking)\n");
            g_bFastMalloc = FALSE;
            g_bFastMallocAll = FALSE;
        }
        //If there are a lot of leaks it is possible to heartbeat fail in
        //the middle of dumping leaks.
        if (g_bDisableHeartbeat == FALSE)
        {
            suprintf("Option: Disable HeartBeat (--dhb option forced for "
                    "leak-checking)\n");
            g_bDisableHeartbeat = TRUE;
        }
    }

    //if (g_bReportRSSFlag)
    if (RSSManager::m_bRSSReportEnabled)
    {
        // We delay printing this since it is common for people to use
        // both --debug and --rss, and without this the option is displayed
        // twice which is one of those small things that just annoys people.
        suprintf("Option: Report Server Stats (%d seconds)\n",
                RSSManager::m_ulRSSReportInterval);
    }

    if (g_ulMaxCrashAvoids)
    {
        // Similarly, we delay printf this so it prints only once when
        // people use both --debug and --mca.
        suprintf("Option: Maximum Crash Avoids = %u\n", g_ulMaxCrashAvoids);
    }

    if (pszTmpConfigFile)
    {
        suprintf("Using Config File: %s\n", pszTmpConfigFile);
    }
    else
    {
        fprintf (stderr, "***You must specify a configuration file.***\n\n");
        terminate(1);
    }

    fflush(0);
    
    //Detect and report the OS info
    if (OSDetect(g_bUnsupportedOSFlag) == 0 && !g_bUnsupportedOSFlag)
    {
        printf("FATAL ERROR -- WRONG OS DETECTED, EXITING...\n\n");
        fflush(0);
        terminate(1);
    }


    MonitorAliveChecker.Init();
    MonitorAliveChecker.SetBlocking(FALSE);

#ifdef _UNIX
    socketpair(AF_UNIX, SOCK_STREAM, 0, MonitorControllerSocket);

    if (g_bDoSizes)
    {
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
	sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))_MMreportsizes;
	sigaction(CHECKSIZES_SIGNAL, &sa, NULL);
	sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))_MMresetsizes;
	sigaction(RESETSIZES_SIGNAL, &sa, NULL);
	sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))_MMstopsizes;
	sigaction(STOPSIZES_SIGNAL, &sa, NULL);
    }
    /*
     * Parent /Child Process Diagram
     *
     *              Monitor PID
     *              .       .
     *             .         .
     *    Alive Checker    Server's Controller
     *                      /  /  |  |  \  \
     *                     /  /   |  |   \  \
     *                    More Server Processes
     *
     * "." = divorced via setsid()
     */
    struct passwd* pPwEnt = getpwuid(geteuid());
    const char* pUsername = pPwEnt ? pPwEnt->pw_name : 0;

    g_MonitorPID = getpid();
    monitor_reported_controller_pid = fork();
    if (monitor_reported_controller_pid > 0)
    {
        /* Pass all these signals to the Controller */
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_flags = SA_SIGINFO | SA_ONSTACK;

	sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))monitor_pass_handler;
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(RECONFIG_SIGNAL, &sa, NULL);
	sigaction(RESTART_SIGNAL, &sa, NULL);

	sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))monitor_dump_trace;
	sigaction(SIGXFSZ, &sa, NULL);

	sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))monitor_alive_restarter;
	sigaction(SIGUSR1, &sa, NULL);

        /* Parent (Monitor Process) */
        if ((g_AliveCheckerPID = fork()) == 0)
        {
            /* Child: Alive Checker */
            AliveChecker(&MonitorAliveChecker);
            /* Never to return... */
            HX_ASSERT(0);
        }

        /* Handle these approriately */
	sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))monitor_child_handler;
	sigaction(SIGCHLD, &sa, NULL);
	sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);

        while (1)
        {
            UINT8 temp[32];
            int n = read(MonitorControllerSocket[0], temp, 1);
            if (n > 0)
            {
                fflush(0);
                if (n == 1 && temp[0] == 'R')
                {
                    /* Restart the server */
                    kill(g_AliveCheckerPID, SIGUSR1);
                    close(MonitorControllerSocket[0]);
                    close(MonitorControllerSocket[1]);
                    MonitorAliveChecker.Close();

                    /* Return to original UID/GID for Restart */
                    setuid(getuid());
                    setgid(getgid());

                    /* Hold on for a second and absorb signals */
                    int s = 3;
                    while (s = sleep(s));

                    int iret = execvp(restartargv[0], (char **)restartargv);
                    if (iret == -1)
                    {
                        printf("Failed exec %d!!!\n", errno);
                        fflush(0);
                    }
                }
                else if (n == 1 && temp[0] == 'S')
                {
                    /* Shutdown Cleanly */
                    struct rlimit r = {0, 0};
                    setrlimit(RLIMIT_CORE, &r);
                    kill(g_AliveCheckerPID, SIGUSR1);
                    exit(0);
                }
                else if (n == 1 && temp[0] == 'C')
                {
		    g_bForceRestart = TRUE;

                    if (!alive_restart)
                    {
                        /*
                         * Remove --rro from restartargv in preparation for a clean
                         * restart.
                         */
                        char **pArgs;
                        BOOL bRemove = FALSE;
                        for (pArgs = (char **)restartargv; *pArgs; pArgs++)
                        {
                            if (!strcmp(*pArgs, "--rro"))
                            {
                                bRemove = TRUE;
                            }
                            if (bRemove && *pArgs)
                            {
                                *pArgs = *(pArgs + 1);
                            }
                        }
                    }
                }
                else if (n == 1 && temp[0] == 'U')
                // set its UID to the one in the config file
                {
                    UINT32 s_int = sizeof(UINT32);
                    n = read(MonitorControllerSocket[0], temp, s_int);
                    fflush(0);
                    if (n == (int)s_int)
                    {
                        UINT32 uid = getlong(temp);
#if defined (_AIX) || defined (_HPUX)
                        if (setreuid(-1, uid) < 0)
#else
                        if (seteuid(uid) < 0)
#endif
                            perror("seteuid() failed(2)");
                        MonitorAliveChecker.Write((unsigned char*)"U", 1);
                        MonitorAliveChecker.Write(temp, s_int);
                    }
                }
                else if (n == 1 && temp[0] == 'G')
                // set its GID to the one in the config file
                {
                    UINT32 s_int = sizeof(UINT32);
                    n = read(MonitorControllerSocket[0], temp, s_int);
                    fflush(0);
                    if (n == (int)s_int)
                    {
                        UINT32 gid = getlong(temp);
#if defined(_AIX) || defined (_HPUX)
                        if (setregid(-1, gid) < 0)
#else
                        if (setegid(gid) < 0)
#endif
                            perror("setegid() failed(2)");
                        MonitorAliveChecker.Write((unsigned char*)"G", 1);
                        MonitorAliveChecker.Write(temp, s_int);
                    }
                }
                else if (n == 1 && temp[0] == 'P')
                {
                    int s_int = sizeof(int);
                    n = read(MonitorControllerSocket[0], temp, s_int);
                    fflush(0);
                    if (n == (int)s_int)
                    {
                        /* Relay this message to the Alive Checker Process */
                        MonitorAliveChecker.Write((unsigned char*)"P", 1);
                        MonitorAliveChecker.Write(temp, s_int);
                    }
                }
                else if (n == 1 && temp[0] == 'D')
                {
                    int s_int = sizeof(int);
                    n = read(MonitorControllerSocket[0], temp, s_int);
                    fflush(0);
                    if (n == (int)s_int)
                    {
                        /* Relay this message to the Alive Checker Process */
                        MonitorAliveChecker.Write((unsigned char*)"D", 1);
                        MonitorAliveChecker.Write(temp, s_int);
                    }
                }
                else
                {
                    if (n > 0)
                    {
                        PANIC(("Internal Error m/1529 -- %d %c\n", n, temp[0]));
                        fflush(0);
                    }
                }
            }
        }
    }
    else if (monitor_reported_controller_pid < 0)
    {
        switch(errno)
        {
            case EAGAIN:
                suprintf("<%s> has too many forked processes!\n",
                    pUsername ? pUsername : "User");
                break;
            case ENOMEM:
                suprintf("fork() failed due to INSUFFICIENT SWAP space!\n");
                break;
            default:
                suprintf("fork() failed!\n");
                break;
        }
        fflush(0);
    }
#else /* for losedoes */
    DWORD dwSomeMSParam = 0;
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AliveCheckerThread,
        &MonitorAliveChecker, NULL, &dwSomeMSParam);
#endif

#ifdef PTHREADS_SUPPORTED
#ifdef _LINUX
    suprintf("Starting PID %lu TID %lu/%d, procnum 0 (controller)\n",
           getpid(), pthread_self(), gettid());
#else
    suprintf("Starting PID %lu TID %lu, procnum 0 (controller)\n",
           getpid(), pthread_self());
#endif
#endif

    Process* proc;
    int procnum;
    DispatchQueue* dq;
    int *s;
    int p[2] = { PROC_RM_CONTROLLER, PROC_RM_CORE };

#ifdef _WIN32

    if (!g_bNoCrashAvoidance)
    {
        SetUnhandledExceptionFilter(
            (LPTOP_LEVEL_EXCEPTION_FILTER)server_fault);
        _set_purecall_handler(PurecallHandler);
    }

#endif

#ifdef _UNIX

#if defined _LINUX || defined PTHREADS_SUPPORTED
    // set the bos to the address of any stack variable, it's close enough
    controllerBOS = (char*)&procnum;
    g_pStackBase = controllerBOS;

    // init stack array and controller's entry in it
    memset(g_threadStacks, 0, MAX_THREADS * sizeof(StackUsage));
    g_threadStacks[0].pStack = controllerBOS;

#endif // _LINUX || PTHREADS_SUPPORTED

    memset(g_GlobalProcessList, 0, MAX_THREADS * sizeof(int));
    memset(g_GlobalProcListFiles, 0, MAX_THREADS * sizeof(char*));
    AddToProcessList(getpid());

    if (g_bAllowCoreDump == FALSE)
    {
        struct rlimit r = {0, 0};
        setrlimit(RLIMIT_CORE, &r);
    }
#ifdef RLIM_INFINITY
    else
    {
        struct rlimit r = {RLIM_INFINITY, RLIM_INFINITY};
        setrlimit(RLIMIT_CORE, &r);
    }
#endif
    setsid();
 
    // block SIGALRM for all children of the controller process.
    // the timer thread has to unblock the signal and setup the handler
    sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, NULL);

    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;

    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))gracefully_killme;
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))dump_trace;
    sigaction(SIGXFSZ, &sa, NULL);

    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);

    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))hup_handler;
    sigaction(RECONFIG_SIGNAL, &sa, NULL);

    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))handle_child;
    sigaction(SIGCHLD, &sa, NULL);

    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))cont_handler;
    sigaction(RESTART_SIGNAL, &sa, NULL);
#if defined PAULM_REPORT
    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))_SocketTimes;
    sigaction(SOCKETTIMES_SIGNAL, &sa, NULL);
#endif
    if (!g_bNoCrashAvoidance)
    {
#if defined _LINUX || defined _SOLARIS || defined _AIX
	sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))server_fault;
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGBUS,  &sa, NULL);
	sigaction(SIGIOT,  &sa, NULL);
	sigaction(SIGFPE,  &sa, NULL);
#if !defined(MEMCACHE_DUMP)
	sigaction(SIGILL,  &sa, NULL);
#endif // MEMCACHE_DUMP
#endif // defined _LINUX || defined _SOLARIS
  
#if defined _HPUX || defined _AIX
	sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))crash_print;
	sigaction(SIGXCPU, &sa, NULL);
#endif
    }
    pMakeProcess = (void *)MakeProcess;
    g_ControllerPid = getpid();

#ifdef PTHREADS_SUPPORTED
    if (g_bUnixThreads)
        g_ControllerTid = pthread_self();
#endif // PTHREADS_SUPPORTED

    setfdlimit(zMaxOpenFDs);
#endif

#if !defined (DONT_SET_NEW_HANDLER)
#if (defined __GNUC__) && (__GNUC__ < 3)
    set_new_handler(__my_new_handler);
#endif /* gcc < 3 */
#endif /* DONT_SET_NEW_HANDLER */

#if defined _UNIX
    // reset the flags since it might have been modified for the synchronous
    // signals SEGV, ABRT, etc.
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
#endif

#if defined(__GNUC__) && defined(i386) && defined (DEBUG) && defined(_UNIX)
    setup_trace(argv[0]);

    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))_MMcheckguards;
    sigaction(CHECKGUARDS_SIGNAL, &sa, NULL);
#if !defined(ENABLE_PROFILING)
    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))_MMresetleaks;
    sigaction(RESETLEAKS_SIGNAL, &sa, NULL);
    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))_MMcheckleaks;
    sigaction(CHECKLEAKS_SIGNAL, &sa, NULL);
    if (!g_bDoSizes)
    {
	sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))_MMsuspendleaks;
	sigaction(SUSPENDLEAKS_SIGNAL, &sa, NULL);
    }
#endif
#else
#if (defined _SOLARIS || defined _HPUX || defined _OSF1 || defined _AIX || defined i386) && defined _UNIX && defined PAULM_ALLOCTRACK && !defined(ENABLE_PROFILING)
    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))_MMresetleaks;
    sigaction(RESETLEAKS_SIGNAL, &sa, NULL);
    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))_MMcheckleaks;
    sigaction(CHECKLEAKS_SIGNAL, &sa, NULL);
    if (!g_bDoSizes)
    {
	sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))_MMsuspendleaks;
	sigaction(SUSPENDLEAKS_SIGNAL, &sa, NULL);
    }
#endif
#endif

#if defined(MEMCACHE_DUMP) && defined(_UNIX)
    sa.sa_sigaction = (void(*)(int, siginfo_t *, void *))MemCacheSigHandler;
    sigaction(SIGILL, &sa, NULL);
#endif


    suprintf("Creating Server Space...\n");
    SharedMemory::Create();
    shared_ready = 1;

#if defined PTHREADS_SUPPORTED && defined _LINUX
    g_GlobalThreadList = new unsigned long[MAX_THREADS];
    memset(g_GlobalThreadList, 0, MAX_THREADS * sizeof(unsigned long));
#endif

    g_pBytesServed = new UINT32;
    g_pPPS = new UINT32;
    g_pLiveIncomingPPS = new UINT32;
    g_pOverloads = new UINT32;
    g_pBehind = new UINT32;
    g_pConcurrentOps = new UINT32;
    g_pConcurrentMemOps = new UINT32;
    g_pNoBufs = new UINT32;
    g_pOtherUDPErrs = new UINT32;
    // XXXAAK: the +1 is a flag that gets set each time around when rss executes
    // and it gets reset in the OncePerSecond::Func()
    g_pMainLoops = new UINT32[MAX_THREADS+1];
    g_pAggregateRequestedBitRate = new UINT32;
    g_pSchedulerElems = new UINT32;
    g_pISchedulerElems = new UINT32;
    g_pTotalNetReaders = new UINT32;
    g_pTotalNetWriters = new UINT32;
    g_pMutexNetReaders = new UINT32;
    g_pMutexNetWriters = new UINT32;
    g_pIdlePPMs = new UINT32;
    g_pWouldBlockCount = new UINT32;
    g_pSocketAcceptCount = new UINT32;
    g_pAggregatablePlayers = new UINT32;
    g_pFileObjs = new UINT32;
    g_pForcedSelects = new UINT32;
    g_pProcessStartInProgress = new BOOL;
    g_pCPUCount = new UINT32;
    g_pNumStreamers = new UINT32;
    g_pResends = new UINT32;
    g_pAggreg = new UINT32;
    g_pIsForcedSelect = new UINT32;
    g_pNow = new Timeval;
    g_bITimerAvailable = new BOOL;
    g_pNumCrashAvoids = new UINT32;
    g_pNumCrashAvoidsInWindow = new UINT32;
    g_pCrashAvoidWindowTime = new Timeval;
    g_pBroadcastRecvBytes = new double;
    g_pBroadcastDistBytes = new double;
    g_pBroadcastRecvPackets = new UINT32;
    g_pBroadcastDistPackets = new UINT32;
    g_pBroadcastPacketsDropped = new UINT32;
    g_pBroadcastPPMOverflows = new UINT32;
    g_bLimitParallelism = new BOOL;
    g_pClientGUIDTable = new ClientGUIDTable;
    g_pMemoryMappedDataSize = new UINT32;
    g_bMmapGrowsDown = new BOOL;

    g_pProtMgr = new HXProtocolManager();
    g_pProtMgr->Init();

    g_pStreamerCount = new UINT32;
    g_pNewConnAllow = new BOOL;
    g_pSecondsToShutdown = new UINT32;
    g_bEnableClientConnCleanup = new BOOL;
    *g_bEnableClientConnCleanup = FALSE;
    g_pClientTerminationBySelf = new UINT32;
    g_pClientTerminationForced = new UINT32;
    g_pClientIgnoreTermReq = new UINT32;
    g_pSignalCount = new UINT32;
    *g_pSignalCount = 0;

#if defined(_LINUX)
    g_pSysInfo = new CLinuxSysInfo();
#elif defined (_FREEBSD) || defined (_OPENBSD) || defined (_NETBSD)
    g_pSysInfo = new CBSDSysInfo();
#elif defined (_HPUX)
    g_pSysInfo = new CHPSysInfo();
#elif defined (_AIX)
    g_pSysInfo = new CAIXSysInfo();
#elif defined (_SOLARIS)
    g_pSysInfo = new CSolarisSysInfo();
#elif defined (_OSF1)
    g_pSysInfo = new COSF1SysInfo();
#elif defined (_WINDOWS)
    g_pSysInfo = new CWindowsSysInfo();
#endif

    // Packet aggregation default paramaters, see ppm.cpp
    g_pAggregateTo      = new UINT32[3];
    g_pAggregateHighest = new UINT32[3];
    g_pAggregateMaxBPS  = new UINT32[3];
    g_pAggregateTo[0]      = 200;
    g_pAggregateHighest[0] = 300;
    g_pAggregateMaxBPS[0]  = 0;
    g_pAggregateTo[1]      = 700;
    g_pAggregateHighest[1] = 1000;
    g_pAggregateMaxBPS[1]  = 100000;
    g_pAggregateTo[2]      = 1000;
    g_pAggregateHighest[2] = 1350;
    g_pAggregateMaxBPS[2]  = 0;

    *g_pBytesServed = 0;
    *g_pPPS = 0;
    *g_pLiveIncomingPPS = 0;
    *g_pOverloads = 0;
    *g_pBehind = 0;
    *g_pNoBufs = 0;
    *g_pOtherUDPErrs = 0;
    memset(g_pMainLoops, 0, sizeof(UINT32) * MAX_THREADS+1);
    *g_pAggregateRequestedBitRate = 0;
    *g_pConcurrentOps = 0;
    *g_pConcurrentMemOps = 0;
    *g_pSchedulerElems = 0;
    *g_pISchedulerElems = 0;
    *g_pTotalNetReaders = 0;
    *g_pTotalNetWriters = 0;
    *g_pMutexNetReaders = 0;
    *g_pMutexNetWriters = 0;
    *g_pIdlePPMs = 0;
    *g_pWouldBlockCount = 0;
    *g_pSocketAcceptCount = 0;
    *g_pAggregatablePlayers = 0;
    *g_pFileObjs = 0;
    *g_pForcedSelects = 0;
    *g_pProcessStartInProgress = FALSE;
    *g_pCPUCount = 1;
    *g_pNumStreamers = 1;
    *g_pResends = 0;
    *g_pAggreg = 0;
    *g_pIsForcedSelect = 0;
    *g_bITimerAvailable = FALSE;
    *g_pNumCrashAvoids = 0;
    *g_pNumCrashAvoidsInWindow = 0;
    memset(g_pCrashAvoidWindowTime, 0, sizeof(Timeval));
    *g_pBroadcastRecvBytes = 0;
    *g_pBroadcastDistBytes = 0;
    *g_pBroadcastRecvPackets = 0;
    *g_pBroadcastDistPackets = 0;
    *g_pBroadcastPacketsDropped = 0;
    *g_pBroadcastPPMOverflows = 0;
    *g_bLimitParallelism = FALSE;
    *g_pMemoryMappedDataSize = 0;
    *g_bMmapGrowsDown = TRUE;

    g_pbTrackAllocs = new BOOL;
    *g_pbTrackAllocs = FALSE;

    g_pbTrackFrees= new BOOL;
    *g_pbTrackFrees= FALSE;

    *g_pStreamerCount = 0;

    // we had to defer setting up the signal stack for this proc
    // until we had the shared memory to set up the AllocateStack mutex.

#if defined _LINUX || defined PTHREADS_SUPPORTED
    g_pAllocateStackMutex = HXCreateMutex();
#endif
#if defined _LINUX || defined _SOLARIS || defined _AIX
    if (!g_bNoCrashAvoidance)
    {

        // set up initial thread's/process's signal stack
        stack_t ss;
        memset(&ss, 0, sizeof(stack_t));
        ss.ss_sp = AllocateStack();

#if defined _LINUX || defined PTHREADS_SUPPORTED
        STACK_DATA_TYPE* pData = new STACK_DATA_TYPE;
        memset(pData, 0, sizeof(STACK_DATA_TYPE));
        pData->processname = "controller";
        *((void**)ss.ss_sp - 1) = pData;
        ss.ss_size = STACKSIZE - sizeof(STACK_DATA_TYPE*) - GUARD_SIZE;
#else
        ss.ss_size = STACKSIZE;
#endif // _LINUX

        ss.ss_sp = (char*)ss.ss_sp - STACKSIZE;
        sigaltstack(&ss, NULL);
    }
#endif

    g_pServerMainLock = HXCreateMutex();
    HXMutexLock(g_pServerMainLock);
#ifdef ENABLE_PROFILING
    g_pProfilingLock = HXCreateMutex();
#endif
    g_pCrashPrintLock = HXCreateMutex();

#ifdef HXATOMIC_MUTEX_POOL_SIZE
    g_AtomicOps.InitLockPool(); //initialize HXAtomic mutex pool
#endif

    // Create a new Dispatch Queue
    dq = new DispatchQueue;
#ifdef PAULM_HXBUFFERSPACE
    g_pCHXBufferTotalData = new UINT32;
    *g_pCHXBufferTotalData = 0;
#endif
#ifdef PAULM_HXSTRINGSPACE
    g_pCHXStringTotalData = new UINT32;
    *g_pCHXStringTotalData = 0;
#endif

    /*
     * Allocate space for global vars here, before
     * other processes are created.
     */
    InitServerResourceValues();
#ifdef _UNIX
    g_pbHandleCHLD = new BOOL;
    *g_pbHandleCHLD = TRUE;
    g_pbRestarting = new BOOL;
    *g_pbRestarting = FALSE;
    g_ppLastEngine = new Engine*;
    *g_ppLastEngine = 0;
    if (g_bDoSizes)
    {
        g_pSizes = new UINT32[65536];
        SharedMemory::resetsizes();
    }
#endif
    g_ppLastEngine = new Engine*;
    *g_ppLastEngine = 0;
    g_pNAKCounter = new UINT32;
    *g_pNAKCounter = 0;

#if defined(_LINUX) && !defined(PTHREADS_SUPPORTED)
    // This is used by platform/unix/pthread_compat.cpp, not by regular pthreads
    memset((void*)g_PThreadMutexCleanupStack, 0, MAX_THREADS * sizeof(CHXSimpleList*));
#endif //defined(_LINUX) && !defined(PTHREADS_SUPPORTED)

    if (pszTmpConfigFile) g_pszConfigFile = new_string (pszTmpConfigFile);
    if (pszTmpImportKey)  g_pszImportKey  = new_string (pszTmpImportKey);

    procnum = Process::GetNewProcessNumber();
    g_nTLSProcnum = procnum;

    Process::SetupProcessIDs();

    /*
     * globals for crash avoidance
     */
    g_dq = dq;

    pTimerProcInitMutex = new int;
    pMiscProcInitMutex = new int;
    pResolverProcInitMutex = new int;
    pStreamerProcInitMutex = new int;
    pHXProcInitMutex = new int;
    pMemreapProcInitMutex = new int;

    proc = new Process;
    proc->AssignProcessNumber(procnum);
#ifdef _WIN32
    proc->AssignCurrentHandle();
#endif // _WIN32

    CoreProcessInitCallback* cb = new CoreProcessInitCallback;
    cb->argc                        = argc;
    cb->argv                        = argv;
    cb->controller_ready            = new int;
    cb->controller_waiting_on_core  = new int;
    cb->return_uid                  = new UINT32((UINT32)-1);
    cb->return_gid                  = new UINT32((UINT32)-1);
    cb->core_proc                   = new Process*;
    *cb->controller_ready           = 0;
    *cb->controller_waiting_on_core = 1;
    cb->dispatch_queue              = dq;

    *pTimerProcInitMutex = 1;
    UnixTimerInitCallback* tcb = new UnixTimerInitCallback;
    MakeProcess("timer", tcb, dq, 0);
    while (*pTimerProcInitMutex)
        microsleep(1);

#ifdef PAULM_ALLOCTRACK
    if (g_bOgreDebug)
    {
        SharedMemory::setogredebug(TRUE);
    }
#endif

    s = MakeProcess("core", cb, dq, CP_CREATE_SOCKET);

    // Wait until the core is done with all it's work
    while (*cb->controller_waiting_on_core)
        microsleep(1);

    // copy the core pc to the controller

    proc->pc = new ControllerContainer(proc, (*cb->core_proc)->pc);
    proc->pc->process_type = PTController;

#ifdef _UNIX
    if (*cb->return_gid != (UINT32)-1)
    {
#if defined (_AIX) || defined (_HPUX)
        setregid(-1, *cb->return_gid);
#else
        setegid(*cb->return_gid);
#endif
        // send G to monitor followed by the new GID
        write(MonitorControllerSocket[1], "G", 1);
        UINT8 temp[sizeof(UINT32)];
        putlong(temp, *cb->return_gid);
        write(MonitorControllerSocket[1], (const void *)temp, sizeof(UINT32));
    }
    if (*cb->return_uid != (UINT32)-1)
    {
#if defined _AIX || defined _HPUX
        setreuid(-1, *cb->return_uid);
#else
        seteuid(*cb->return_uid);
#endif
        // send U to monitor followed by the new UID
        write(MonitorControllerSocket[1], "U", 1);
        UINT8 temp[sizeof(UINT32)];
        putlong(temp, *cb->return_uid);
        write(MonitorControllerSocket[1], (const void *)temp, sizeof(UINT32));
    }
#endif

    delete cb->controller_waiting_on_core;
    delete cb->return_uid;
    delete cb->return_gid;
    delete cb->core_proc;

    /*
     * Now that the proc->pc is established, it is safe to initialize the
     * IHXNetworkServicesContext
     */

    proc->pc->network_services->Init(proc->pc->server_context,
                                     proc->pc->engine, NULL);

    proc->pc->net_services->Init(proc->pc->server_context);

#if !defined WIN32
#ifdef SHARED_FD_SUPPORT
    if (!g_bSharedDescriptors)
#endif /* SHARED_FD_SUPPORT */
    {
        ((ControllerContainer *)proc->pc)->fdp_socket = new FDPassSocket(s, p, proc);
    }
#endif

    proc->pc->dispatchq->init(proc);

    *cb->controller_ready           = 1;

    proc->pc->rss_manager       = new RSSManager(proc);

    /*
     * OncePerSecond is responsible for calling ServerInfo->Func() and
     * LoadInfo->Func() once per second.
     */
    new OncePerSecond(proc);

    // Create RSS Core Report
    RSSCoreStatsReport* rsscsr = new RSSCoreStatsReport(proc);

    HXMutexUnlock(g_pServerMainLock);

    proc->pc->engine->mainloop();

    return 0;
}

// DisableAliveChecker is the standard way to disable heartbeat checks,
// for instance when --dhb or --lct options are used.
void
DisableAliveChecker()
{
    UINT8 str[sizeof(UINT32)];
    putlong(str, 0);
#ifdef _UNIX
    write(MonitorControllerSocket[1], "D", 1);
    write(MonitorControllerSocket[1], (const void *)str, sizeof(UINT32));
#else
    MonitorAliveChecker.Write((unsigned char*)"D", 1);
    MonitorAliveChecker.Write(str, sizeof(UINT32));
#endif
}

// StopAliveChecker is used to prevent an already running alive checker from
// HBFing when the server is shutting down. Calling DisableAliveChecker alone
// will not always prevent HBFing if the alive checker is already waiting for
// a response. The alive checker will check g_bStopAliveChecker if it gets to
// the fail state, and will only HBF if it is not set.
void
StopAliveChecker()
{
    g_bStopAliveChecker = TRUE;
    DisableAliveChecker();
}

#ifdef _WIN32
void
DumpThreadStack(HANDLE h)
{
    CONTEXT x;
    SuspendThread(h);
    x.ContextFlags = CONTEXT_FULL;
    if (GetThreadContext(h, &x))
    {
        UINT32 ulAt = x.Eip;
        UINT32* pFrame;
        UINT32* pPrevFrame;
        pFrame = (unsigned long *)x.Ebp;
        char buffer[4096];
        while (1)
        {
            sprintf(buffer, "0x%x\n", ulAt);
            SDEM_PRINT(("%s", buffer));
            RSSManager::CrashOutput(buffer);
            if (IsBadWritePtr(pFrame, sizeof(void*)*2))
                break;
            ulAt = pFrame[1];
            pPrevFrame = pFrame;
            pFrame = (UINT32*)pFrame[0];
            if ((UINT32)pFrame & 3)
                break;
            if (pFrame <= pPrevFrame)
                break;
        }
        SDEM_PRINT(("\n\n"));
        RSSManager::CrashOutput("\n\n");
    }
    else if (h != 0)
    {
        char buffer[1024];
        sprintf(buffer, "could not get thread context of 0x%x\n", h);
        SDEM_PRINT(("%s", buffer));
        RSSManager::CrashOutput(buffer);
    }
    ResumeThread(h);
}


void
DumpThreadState()
{
    char buffer[4096];

    for (int i = 0; i < *Process::_numprocs; i++)
    {
        sprintf(buffer, "\n------------------------ Procnum %d TID %d -------------------------\n"
            "Heartbeat Stack Trace:\n", i, Process::_ptids[i]);
        SDEM_PRINT(("%s", buffer));
        RSSManager::CrashOutput(buffer);
        DumpThreadStack((HANDLE)Process::_ptandles[i]);
    }
}


void
CrashThread(HANDLE h)
{
    if (!SuspendThread(h))
    {
        CA_PRINT(("Could not suspend the thread!\n"));
        RSSManager::CrashOutput("Could not suspend the thread!\n");
    }
    CONTEXT x;
    x.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(h, &x))
    {
        char buffer[256];
        sprintf(buffer, "Failed to get thread context of 0x%x\n", h);
        CA_PRINT(("%s", buffer));
        RSSManager::CrashOutput(buffer);
        ResumeThread(h);
        return;
    }
    /*
     * Die
     */
    x.Eip = 0x000000;
    if (!SetThreadContext(h, &x))
    {
        ResumeThread(h);

        char buffer[256];
        sprintf(buffer, "Could not set the thread context of 0x%x\n", h);
        CA_PRINT(("%s", buffer));
        RSSManager::CrashOutput(buffer);
        return;
    }
    ResumeThread(h);
}

#endif

PortWatcher::PortWatcher(Process* pProc)
{
    m_pProc = pProc;

    pProc->pc->server_context->QueryInterface(IID_IHXRegistry,
        (void **)&m_pReg);

    INT32 ulValue;
    m_pReg->GetIntByName("config.RTSPPort", ulValue);

    UINT8 str[sizeof(UINT32)];
    putlong(str, ulValue);
#ifdef _UNIX
    write(MonitorControllerSocket[1], "P", 1);
    write(MonitorControllerSocket[1], (const void *)str, sizeof(UINT32));
#else
    MonitorAliveChecker.Write((unsigned char*)"P", 1);
    MonitorAliveChecker.Write(str, sizeof(UINT32));
#endif

    if (g_bDisableHeartbeat)
    {
        DisableAliveChecker();
    }

    IHXPropWatch* pWatch;
    m_pReg->CreatePropWatch(pWatch);

    pWatch->Init(this);
    pWatch->SetWatchByName("config.RTSPPort");
}

STDMETHODIMP
PortWatcher::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPropWatchResponse))
    {
        AddRef();
        *ppvObj = (IHXPropWatchResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
PortWatcher::AddRef()
{
    /* No Delete */
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
PortWatcher::Release()
{
    /* No Delete */
    return HXR_OK;
}

STDMETHODIMP
PortWatcher::AddedProp(const UINT32            ulId,
                       const HXPropType       propType,
                       const UINT32            ulParentID)
{
    return HXR_OK;
}

STDMETHODIMP
PortWatcher::ModifiedProp(const UINT32            ulId,
                          const HXPropType       propType,
                          const UINT32            ulParentID)
{
    INT32 ulValue;
    m_pReg->GetIntById(ulId, ulValue);

    UINT8 x[sizeof(UINT32)];
    putlong(x, ulValue);
#ifdef _UNIX
    write(MonitorControllerSocket[1], "P", 1);
    write(MonitorControllerSocket[1], (const void *)x, sizeof(UINT32));
#else
    MonitorAliveChecker.Write((unsigned char*)"P", 1);
    MonitorAliveChecker.Write(x, sizeof(UINT32));
#endif

    return HXR_OK;
}

STDMETHODIMP
PortWatcher::DeletedProp(const UINT32            ulId,
                         const UINT32            ulParentID)
{
    return HXR_OK;
}


