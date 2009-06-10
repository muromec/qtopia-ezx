/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: trace.h,v 1.7 2006/01/27 00:36:28 tknox Exp $
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

#ifndef _TRACE_H_
#define _TRACE_H_

#if defined _FREEBSD || defined _LINUX || defined _OPENBSD || defined _NETBSD
#include <sys/types.h> /* for caddr_t */
#endif

#if defined _LINUX || defined _SOLARIS || defined _AIX
#include <ucontext.h>
#endif /* _LINUX || _SOLARIS || _AIX */

extern "C"
{
#if defined(__GNUC__) && defined(i386) && defined(DEBUG) && \
    (defined(_FREEBSD) || defined(_OPENBSD) || defined(_NETBSD))
    char* find(caddr_t addr, int moo);
    void setup_trace(char *prog);
#elif defined(DEBUG) && defined(_LINUX)
    void setup_trace(char *prog);
    char* find(caddr_t addr, int moo);
    extern "C" void CrashOutputWrapper(const char *pString);
    void get_trace_from_eip_and_ebp(unsigned long eip, unsigned long ebp,
        unsigned long bos);
    char* get_trace_from_context(ucontext* pC);
#elif !defined(DEBUG) && defined(i386) && defined(_UNIX)
    void setup_trace(char *prog);
    char* find(unsigned long addr);
#if defined(_LINUX)
    extern "C" void CrashOutputWrapper(const char *pString);
    void get_trace_from_eip_and_ebp(unsigned long eip, unsigned long ebp,
        unsigned long bos);
    char* get_trace_from_context(ucontext* pC);
#endif
#elif defined(_LINUX)   // Catch-all for any remaining undetected Linux
    void setup_trace(char *prog);
    char* find(caddr_t addr, int moo);
    extern "C" void CrashOutputWrapper(const char *pString);
    void get_trace_from_eip_and_ebp(unsigned long eip, unsigned long ebp,
        unsigned long bos);
    char* get_trace_from_context(ucontext* pC);
#elif defined _SOLARIS
    char* find(unsigned long addr);
    void print_frame(void);
    char* get_trace_from_context(ucontext_t* pC);
#elif defined _HPUX
    void get_ustack_trace();
#elif defined (_OSF1)
    void __init_get_trace(void);
#elif defined (_AIX)
    void setup_trace(char *prog);
    char* find(unsigned long addr);
    char* get_trace_from_context(ucontext_t* pC);
#elif defined _WIN32
    void setup_trace(char *prog);
    char* find(unsigned long addr);
#else
    void setup_trace(char *prog);
    char* find(unsigned long addr);
#endif

    char* get_trace_from_stack(void** stack, int size);
    char* get_trace();
    void** get_stack(int* size);
}

#endif
