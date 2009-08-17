/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: main.cpp,v 1.10 2005/08/15 21:29:14 atin Exp $
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

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#if defined _AIX
#include <new.h>
#endif


#include "hxtypes.h"
#include "platform_config.h"
#include "hxcom.h"
#include "debug.h"
#include "hxproc.h"
#include "proc.h"
#include "shmem.h"
#include "config.h"
#include "mem_cache.h"

extern int shared_ready;
extern BOOL g_bFastMallocAll;


// GCC 3.x uses a different symbol than 2.x, and it resides in libstdc++.
// We don't link with libstdc++ so we must overload it here.
#if defined(__GNUC__) && (__GNUC__ == 3)

// Don't use the old GCC 2.x overload
#ifndef _DONT_OVERLOAD_PURE_VIRTUAL
#define _DONT_OVERLOAD_PURE_VIRTUAL
#endif

#define _OVERLOAD_CXA_PURE_VIRTUAL

#endif

#ifdef _UNIX

// just more readable. hence the seemingly unnecessary ifdef
#ifdef _DONT_OVERLOAD_PURE_VIRTUAL
#else

// overloading __pure_virtual does not work with gcc 2.95.x

extern "C"
{
    void __pure_virtual();
}

void
__pure_virtual()
{
    printf ("Pure virtual function called\n");
    volatile int* x = 0;
    *x = 5;
}

#endif /* _DONT_OVERLOAD_PURE_VIRTUAL */

#ifdef _OVERLOAD_CXA_PURE_VIRTUAL //needed for Linux-2.4 w/ gcc-3.0
extern "C" void __cxa_pure_virtual();
void
__cxa_pure_virtual()
{
    printf ("Pure virtual function called\n");
    volatile int* x = 0;
    *x = 5;
}
#endif

#endif /* _UNIX */

#if defined(_LINUX) && !defined(PTHREADS_SUPPORTED) && !defined(_RHEL3)
//this forces the use of LinuxThreads rather than NTPL
extern int errno;
int stub_errno()
{
    return errno;
}
#endif

int
_main(int argc, char** argv);

char * VOLATILE * VOLATILE restartargv;
char * VOLATILE * VOLATILE origargv;
int origargc;
extern char **environ;
extern int global_do_config_reload;
int origstderr;

void*
operator new(size_t size)
{
    if (g_pMemCacheList && g_bFastMallocAll && shared_ready)
    {
        // We can just use 0 since CacheNew() looks up and calls
        // the correct, thread/process-specific, MemCache object.
        MemCache* pMemCache = g_pMemCacheList[0];
        if (pMemCache)
        {
            return (void*)(pMemCache->CacheNew(size));
        }
    }

    if (shared_ready)
        return SharedMemory::malloc(size);
    else
        return (void *)malloc(size);
}

void
operator delete(void *ptr)
{
    if (ptr == NULL)
        return;

    if (g_pMemCacheList && g_bFastMallocAll && shared_ready)
    {
        MemCache* pMemCache = g_pMemCacheList[0];
        if (pMemCache)
        {
            pMemCache->CacheDelete((char*)ptr);
            return;
        }
    }

    if (shared_ready)
        SharedMemory::free((char*)ptr);
    else
        free(ptr);
}

#ifdef _UNIX
void*
operator new [] (size_t size)
{
    if (g_pMemCacheList && g_bFastMallocAll && shared_ready)
    {
        MemCache* pMemCache = g_pMemCacheList[0];
        if (pMemCache)
            return (void*)(pMemCache->CacheNew(size));
    }

    if (shared_ready)
        return SharedMemory::malloc(size);
    else
        return (void *)malloc(size);
}

void
operator delete [] (void *ptr)
{
    if (ptr == NULL)
        return;

    if (g_pMemCacheList && g_bFastMallocAll && shared_ready)
    {
        MemCache* pMemCache = g_pMemCacheList[0];
        if (pMemCache)
        {
            pMemCache->CacheDelete((char*)ptr);
            return;
        }
    }

    if (shared_ready)
        SharedMemory::free((char *)ptr);
    else
        free(ptr);
}
#endif /* _UNIX */

void
procstr(char *src)
{
#if defined _RED_HAT_5_X_ || (!defined(_LINUX) && !defined(_IRIX))
    setprocstr(src, (char **)origargv, origargc, environ);
#endif
}

void
terminate(int code)
{
    char buffer[] = "\n\nErrors occured, check log file\n\n";

    if(origstderr)
        write(origstderr, buffer, sizeof(buffer) - 1);

    killpg(0, SIGTERM);
    shared_ready = 0;
    sleep(30);
    killpg(0, SIGTRAP); // Not Reached
}

void
exit_hook()
{
    DPRINTF(D_INFO, ("(%d) exited\n", getpid()));
}

void
exit_server(int code)
{
    DPRINTF(D_INFO, ("SIGINT or SIGTERM received, code: %d\n", code));
    killpg(0, code);
    shared_ready = 0;
    exit(1);
}

void
handle_pipe(int code)
{
    DPRINTF(D_INFO, ("SIGPIPE received, code: %d\n", code));
}

#if defined(_FREEBSD) && defined (DEBUG)
#include <osreldate.h>
extern FILE* error_file;
extern "C"
{
    extern void malloc_dump(FILE *fd);
}

void
handle_info(int code)
{
    DPRINTF(D_INFO, ("SIGINFO received, code: %d\n", code));
#if (__FreeBSD_version >= 199608)
#endif
}
#endif

void
reload_config(int code)
{
#if XXXSMP
    DPRINTF(D_INFO, ("SIGHUP received, code: %d\n", code));
    global_do_config_reload = 1;
#endif
}

void
ignore_sighup(int code)
{
    DPRINTF(D_INFO, ("SIGHUP received, code: %d\n", code));
}

extern void handle_child(int code);

void
os_init()
{
    origstderr = dup(2);
}

void
child_handler()
{
}

void
os_start()
{
    atexit(exit_hook);
    close (origstderr);         // Shutdown the real stderr
    origstderr = 0;
}


extern "C" int
lib_main(int argc, char **argv)
{
    origargc = argc;
    origargv = argv;
    BOOL bSeenRestart = FALSE;
    int c;

    //make stdout/stderr line-buffered
    char* pBuf1 = new char[4096];
    char* pBuf2 = new char[4096];
    setvbuf(stdout, pBuf1, _IOLBF, 4096);
    setvbuf(stderr, pBuf2, _IOLBF, 4096);

    argv = new char*[argc];
    restartargv = new char*[argc+2];
    for(c = 0; c < argc; c++)
    {
        argv[c] = new char[strlen(origargv[c]) + 2];
        restartargv[c] = new char[strlen(origargv[c]) + 2];
        if (!strcmp(origargv[c], "--rro"))
        {
            bSeenRestart = TRUE;
        }
        strcpy(argv[c], origargv[c]);
        strcpy(restartargv[c], origargv[c]);
    }
    if (bSeenRestart)
    {
        restartargv[c] = 0;
    }
    else
    {
        restartargv[c] = new char[7];
        strcpy(restartargv[c], "--rro");
    }
    restartargv[c + 1] = 0;

    _main(argc, argv);

    return 0;
}
