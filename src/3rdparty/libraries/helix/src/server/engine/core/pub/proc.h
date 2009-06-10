/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: proc.h,v 1.16 2009/05/27 17:45:08 atin Exp $ 
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

#ifndef _PROC_H_
#define _PROC_H_

#include "platform_config.h"

#if defined(_WIN32)
#include "hlxclib/windows.h"
#include <time.h>
#elif defined unix || defined _UNIX
#include <unistd.h>
#endif

#include <string.h>
#include <stdio.h>

#include "debug.h"
#include "proc_container.h"

class ProcessContainer;

#define MAX_THREADS 512

#if defined _WIN32
#define PROCID_TYPE Process*
#else
#define PROCID_TYPE int
#endif

#ifdef _WIN32
extern __declspec(thread) Process* g_pTLSProc;
extern __declspec(thread) int g_nTLSProcnum;
//need the LINUX_EPOLL_SUPPORT check to fix a Producer rembrdcst plugin build-buster
#elif ((defined _LINUX && LINUX_EPOLL_SUPPORT) || (defined _SOLARIS && defined DEV_POLL_SUPPORT)) && defined PTHREADS_SUPPORTED
extern __thread Process* g_pTLSProc;
extern __thread int g_nTLSProcnum;
#else
extern Process* g_pTLSProc;
extern int g_nTLSProcnum;
#endif // _WIN32

#if defined _LINUX || defined PTHREADS_SUPPORTED
#ifndef SHARED_FD_SUPPORT
#define SHARED_FD_SUPPORT
#endif
extern UINT32 g_ulThreadStackSize;
#endif // _LINUX || _SOLARIS


#ifdef SHARED_FD_SUPPORT
extern BOOL g_bSharedDescriptors;
#endif // SHARED_FD_SUPPORT

#ifdef PTHREADS_SUPPORTED
#  include <pthread.h>
#  ifdef _SOLARIS
#    include <thread.h>
#  endif // _SOLARIS
extern BOOL g_bUnixThreads;
#endif // PTHREADS_SUPPORTED

#if defined _LINUX  || defined PTHREADS_SUPPORTED
#  define STACKSIZE g_ulThreadStackSize
#else
#  define STACKSIZE (1024 * 1024)
#endif // _LINUX

#define GUARD_SIZE    (_PAGESIZE)

#ifndef PTHREADS_SUPPORTED
#define STACK_DATA_TYPE linux_fork_data
#endif //PTHREADS_SUPPORTED

#if defined PTHREADS_SUPPORTED || defined _WIN32
class SimpleCallback;
class DispatchQueue;
struct thread_args
{
    char* processname;
    SimpleCallback* cb;
    DispatchQueue* dispatch_queue;
    Process* proc;
    int process_number;
};
#endif // defined PTHREADS_SUPPORTED || defined _WIN32

#ifdef PTHREADS_SUPPORTED
#  define STACK_DATA_TYPE thread_args
#endif // PTHREADS_SUPPORTED

#if defined _LINUX || defined PTHREADS_SUPPORTED

extern char* maxThreadStack;
extern char* controllerBOS;

//
// grabs the process number from the pData pointer that was pushed onto the
// bottom of the thread's stack by linuxfork.  Note STACKSIZE must be a power
// of 2
//
inline
int get_procnum_from_bos()
{
    char x;
    char* sp = &x;

    //
    // we assume that the controller stack is allocated at the top of
    // the process's address space and so all thread stacks will be
    // below it.
    //
    if (sp > maxThreadStack)
        return 0; // controller's procnum

    //
    // otherwise bump the pointer up to the bottom of the stack (stacks grow down)
    // at which point should reside a pointer to STACK_DATA_TYPE
    //
    STACK_DATA_TYPE** ppData =
        (STACK_DATA_TYPE**)(((UINT32)sp | (STACKSIZE - 1)) + 1);
    ppData--;

    return (*ppData)->process_number;
}

#endif // _LINUX || PTHREADS_SUPPORTED

class Process {
public:
    	    	    	    Process();
    void		    AssignProcessNumber(int __procnum);
    static int		    GetNewProcessNumber();
    static void		    SetupProcessIDs();
    int			    procnum()			{ return _procnum;    }
    static VOLATILE int	    numprocs()			{ return *_numprocs;  }
    PROCID_TYPE VOLATILE    procid(int pn)		{ return _procid[pn]; }

    // see the proc.cpp file for an explanation of why these are non-inline.
    static int              get_procnum();
    static Process*         get_proc(); 

#ifdef _UNIX
    static int		    translate_procnum(int pid);
#else
    /*
     * XXXPM the naming conventions of these funcs are backwards.
     */
    static int		    translate_procnum(UINT32 tid);
    HANDLE		    translate_tid(int procnum);
    static int		    set_handle_of_thread(HANDLE, DWORD);
    void                    AssignCurrentHandle();

#endif
    void		    set_engine(ServerEngine* pEngine);
    ServerEngine*	    get_engine(int procnum);

    ProcessContainer*	    pc;
    int			    _procnum;
    static int* VOLATILE    _numprocs;

    // PID or Thread ID (OS Specific)
    static PROCID_TYPE*	VOLATILE    _procid;
    static ServerEngine** VOLATILE _pserver_engines; 
#ifdef _WIN32
    static UINT32* VOLATILE _ptids;
    static HANDLE* VOLATILE _ptandles;
#endif
};

inline
Process::Process()
{
    _procnum = -1;
    pc = 0;
}

inline void
Process::SetupProcessIDs()
{
    Process::_procid = new PROCID_TYPE[MAX_THREADS];
    Process::_pserver_engines = new ServerEngine*[MAX_THREADS];
    memset(Process::_pserver_engines, 0,
	sizeof(ServerEngine*) * MAX_THREADS);
#ifdef _WIN32
    Process::_ptids = new UINT32[MAX_THREADS];
    memset(Process::_ptids, 0,
	sizeof(UINT32) * MAX_THREADS);
    Process::_ptandles = new HANDLE[MAX_THREADS];
    memset(Process::_ptandles, 0,
	sizeof(HANDLE) * MAX_THREADS);
#endif
}

inline void
Process::AssignProcessNumber(int __procnum)
{
    _procnum = __procnum;

#if defined _WIN32

    _procid[_procnum] = this;
    _ptids[_procnum] = GetCurrentThreadId();

#else

#ifdef PTHREADS_SUPPORTED
    if (g_bUnixThreads)
    {
        _procid[_procnum] = pthread_self();
    }
    else
#endif // PTHREADS_SUPPORTED

    {
        _procid[_procnum] = getpid();
    }

#endif
}

inline int
Process::GetNewProcessNumber()
{
    /* Not thread-safe:  Must only be executed by the controller process */

    if (!_numprocs)
    {
	_numprocs = new int;
	*_numprocs = 0;
    }
    return (*_numprocs)++;
}

#ifdef _UNIX
inline int
Process::translate_procnum(int pid)
{
    int i;
    for (i = 0; i < *_numprocs; i++)
    {
	if (_procid[i] == pid)
	{
	    return i;
	}
    }
    return -1;
}

#else

inline int
Process::translate_procnum(UINT32 tid)
{
    int i;
    for (i = 0; i < *_numprocs; i++)
    {
	if (_ptids[i] == tid)
	{
	    return i;
	}
    }
    return -1;
}

inline HANDLE
Process::translate_tid(int procnum)
{
    if (procnum >= *_numprocs)
    {
	return 0;
    }

    return (HANDLE)_ptandles[procnum];
}
#endif

inline void
Process::set_engine(ServerEngine* pEngine)
{
    Process::_pserver_engines[_procnum] = pEngine;
}

inline ServerEngine*
Process::get_engine(int procnum)
{
    return Process::_pserver_engines[procnum];
}

#ifdef _WIN32
inline int
Process::set_handle_of_thread(HANDLE h, DWORD id)
{
    int i;
    for (i = 0; i < *_numprocs; i++)
    {
	if (_ptids[i] == id)
	{
	    _ptandles[i] = h;
	    return 1;
	}
    }
    return 0;
}

inline void
Process::AssignCurrentHandle()
{
    HANDLE hCurThrd = GetCurrentThread();
    HANDLE hThread;
    HANDLE hProc = GetCurrentProcess();
    if(DuplicateHandle(hProc, hCurThrd, hProc, &hThread, THREAD_ALL_ACCESS, 
        FALSE, 0))
    {
        _ptandles[_procnum] = hThread;
    }
}
#endif /* _WIN32 */

#endif
