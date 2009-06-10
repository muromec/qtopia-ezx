/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: trace.c,v 1.15 2008/07/03 21:54:17 dcollins Exp $ 
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

#ifdef _LINUX
#define FRAME_LIMIT 500
#else
#define FRAME_LIMIT 250
#endif /* _LINUX */

#if defined(i386) && defined(_UNIX)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ucontext.h>
#if defined(__i386__)
#if !defined(REG_EBP)
#define REG_EBP 6
#endif
#if !defined(REG_EIP)
#define REG_EIP 14
#endif
#endif
#endif

#if defined(__GNUC__) && defined (DEBUG) && \
    (defined(_FREEBSD) || defined(_OPENBSD) || defined(_NETBSD)) && \
    (defined(i386) || defined(__i386__))

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stab.h>

#include "nm.h"

struct alist	*linelist;
int		len;


char*
find(caddr_t addr, int moo)
{
    int j, bFirst;
    char pName[2048];
    char pWhere[2048];
    char pArgs[2048];
    char pThisArg[2048];
    static char z_pTemp[2048];
    int i = len / 2;
    int step = len / 2;

    for (;;)
    {
#if defined(_FREEBSD) && (!defined(FREEBSD_ELF) && !defined(__ELF__))
	if ((addr >= linelist[i].addr) && (addr <= linelist[i+1].addr))
	{
	    if (!linelist[i].filename)
		return "";

	    strcpy(pName, linelist[i].entry->n_name);
	    *(strstr(pName, ":")) = 0;
	    sprintf(pWhere, "%s:%d", linelist[i].filename, linelist[i].line);
	    pArgs[0] = 0;

	    bFirst = 1;
	    for (j = 1;; j++)
	    {
		if ((linelist[i].entry + j)->n_type == N_PSYM)
		{
		    int foo = (linelist[i].entry + j)->n_value;
		    if (!bFirst)
			strcat(pArgs, ", ");
		    strcpy(pThisArg, (linelist[i].entry + j)->n_name);
		    *(strstr(pThisArg, ":")) = 0;
		    //sprintf (z_pTemp, "%s=%p", pThisArg, (void *)*((int *)(foo + moo)));
		    //strcat(pArgs, z_pTemp);
		    bFirst = 0;
		}
		else
		    break;
	    }
	    sprintf (z_pTemp, "%s(%s) at %s\n", pName, pArgs, pWhere);
	    return z_pTemp;
	}
	else
#endif
	{
            if (linelist == NULL)
            {
		sprintf (z_pTemp, "%p\n", addr);
                return z_pTemp;
            }
	    step = (step + 1) / 2;
	    if (addr > linelist[i].addr)
		i = i + step;
	    else
		i = i - step;
	    if (i >= len)
	    {
		sprintf (z_pTemp, "%p\n", addr);
		return z_pTemp;
	    }
	    if (i < 0)
	    {
		sprintf (z_pTemp, "%p\n", addr);
		return z_pTemp;
	    }
	}
    }
}


char*
get_trace()
{
    void** pCurrentFrame = 0;
    static char trace[32768];

    trace[0] = 0;

    __asm __volatile("movl %%ebp, %0" : "=d" (pCurrentFrame));

    while (*(void **)*pCurrentFrame)
    {
	strcat(trace, find((*(pCurrentFrame + 1)), (int)*pCurrentFrame));
	pCurrentFrame = *pCurrentFrame;
    }
   
    return trace;
}

void**
get_stack(int* size)
{
    void** pCurrentFrame = 0;
    static void* stack[32768];
    int nframes = 0;
    int frame = 0;

    __asm __volatile("movl %%ebp, %0" : "=d" (pCurrentFrame));
    while(*(void **)*pCurrentFrame)
    {
	stack[frame++] = (*(pCurrentFrame + 1));
	pCurrentFrame = *pCurrentFrame;
    }
    *size = frame;
    return stack;
}

char*
get_trace_from_stack(void** stack, int size)
{
    int i;
    static char trace[32768];

    trace[0] = 0;

    for(i = 0; i < size; i++)
    {
	strcat(trace, find(stack[i], 0));
    }
    return trace;
}

void
setup_trace(char *prog)
{
#if (defined(FREEBSD_ELF) || defined(__ELF__))
    linelist = NULL;
    return;
#else
    FILE* f;

    f = fopen(prog, "r");
    setup_stab(prog, f, &linelist, &len);
    fclose(f);
#endif
}

#elif defined(DEBUG) && defined(_LINUX) && !defined(__amd64__)
#if !defined(_LSB)
#include <bfd.h>
#endif
#define __USE_GNU
#include <dlfcn.h>

void
setup_trace(char* prog)
{
}

extern void* pMakeProcess;

struct funclist {
    char* funcname;
    char* fileline;
    unsigned long addr;
};

struct funclist* linelist;
int func_count;

char*
find(unsigned long addr, int moo, char* pName)
{
    Dl_info info;
    char* func;
    if (dladdr((void*)addr, &info) == 0)
    {
        func = "??";
    }
    else
    {
        func = (char*)info.dli_sname;
    }

    if (moo)
    {
        sprintf(pName, "%p: %s %08x %08x %08x %08x %08x %08x\n",
                addr, func,
                (void *)*((int *)(2 * 4 + moo)),
                (void *)*((int *)(3 * 4 + moo)),
                (void *)*((int *)(4 * 4 + moo)),
                (void *)*((int *)(5 * 4 + moo)),
                (void *)*((int *)(6 * 4 + moo)),
                (void *)*((int *)(7 * 4 + moo)));
    }
    else
    {
        sprintf(pName, "%p: %s\n", addr, func);
    }

    return pName;
}

void
get_trace_from_eip_and_ebp(unsigned long eip, unsigned long ebp, unsigned long bos)
{
    void** pCurrentFrame = (void**)ebp;
    char trace[32768];
    char* pFrame;
    int nFrameCount = 0;
    char buffer[32780];

    find(eip, (int)pCurrentFrame, trace);
    pFrame = trace + strlen(trace);

    while (*(void **)*pCurrentFrame && nFrameCount < FRAME_LIMIT)
    {
        find((unsigned long)(*(pCurrentFrame + 1)), (int)*pCurrentFrame, pFrame);
        pFrame += strlen(pFrame);

        // don't allow too many frames to be printed because we don't
        // want to blow out our trace buffer
        nFrameCount++;

        if (((*(pCurrentFrame + 1) > pMakeProcess - 64)) &&
            ((*(pCurrentFrame + 1) < pMakeProcess + 64)))
            break;

        pCurrentFrame = *pCurrentFrame;

        // make sure we aren't looking beyond the end of the stack.  we add
        // some slop due to find, above
        if ((unsigned long)*pCurrentFrame > bos - 32)
            break;
    }
 
    sprintf(buffer, "\n%s\n\n", trace);
    CrashOutputWrapper(buffer);
}

char*
get_trace_from_context(ucontext_t* u)
{
    void** pCurrentFrame = (void**)u->uc_mcontext.gregs[REG_EBP];
    char trace[32768];
    char* pFrame;
    int nFrameCount = 0;
    char buffer[32780];

    find((unsigned long)u->uc_mcontext.gregs[REG_EIP], (int)pCurrentFrame, trace);
    pFrame = trace + strlen(trace);

    while (*(void **)*pCurrentFrame && nFrameCount < FRAME_LIMIT)
    {
	find((unsigned long)(*(pCurrentFrame + 1)), *pCurrentFrame, pFrame);
        pFrame += strlen(pFrame);

        // don't allow too many frames to be printed because we don't
        // want to blow out our trace buffer
        nFrameCount++;
	if (((*(pCurrentFrame + 1) > pMakeProcess - 64)) &&
	    ((*(pCurrentFrame + 1) < pMakeProcess + 64)))
	    break;

	pCurrentFrame = *pCurrentFrame;
    }

    sprintf(buffer, "\n%s\n\n", trace);
    CrashOutputWrapper(buffer);
}

char*
get_trace(void)
{
    void** pCurrentFrame = 0;
    static char trace[32768];
    int nFrameCount = 0;

    trace[0] = 0;

    __asm __volatile("movl %%ebp, %0" : "=d" (pCurrentFrame));

    while (*(void **)*pCurrentFrame && nFrameCount < FRAME_LIMIT)
    {
        find((unsigned long)(*(pCurrentFrame + 1)), (int)*pCurrentFrame, trace + strlen(trace));

        nFrameCount++;
        if (((*(pCurrentFrame + 1) > pMakeProcess - 64)) &&
            ((*(pCurrentFrame + 1) < pMakeProcess + 64)))
            break;

        pCurrentFrame = *pCurrentFrame;
    }
   
    return trace;
}

void**
get_stack(int* size)
{
    void** pCurrentFrame = 0;
    static void* stack[32768];
    int nframes = 0;
    int frame = 0;

    __asm __volatile("movl %%ebp, %0" : "=d" (pCurrentFrame));

    while(*(void **)*pCurrentFrame)
    {
	if (((*(pCurrentFrame + 1) > pMakeProcess - 64)) &&
	    ((*(pCurrentFrame + 1) < pMakeProcess + 64)))
	    break;

	stack[frame++] = (*(pCurrentFrame + 1));
	pCurrentFrame = *pCurrentFrame;
    }
    *size = frame;
    return stack;
}

char*
get_trace_from_stack(void** stack, int size)
{
    int i;
    static char trace[32768];

    trace[0] = 0;

    for(i = 0; i < size; i++)
    {
	find((int)(stack[i]), 0, trace + strlen(trace));
    }
    return trace;
}

#elif !defined(DEBUG) && defined(i386) && defined(_UNIX)

extern void* pMakeProcess;

void
setup_trace(char* prog)
{
}

void
find(unsigned long addr, char* pName)
{
    int i = 0;
    sprintf(pName, "%p\n", (void*)addr);
}

#ifdef _LINUX
void
get_trace_from_eip_and_ebp(unsigned long eip, unsigned long ebp, unsigned long bos)
{
    void** pCurrentFrame = (void**)ebp;
    char trace[32768];
    char* pFrame;
    int nFrameCount = 0;
    char buffer[32780];

    find(eip, trace);
    pFrame = trace + strlen(trace);
    while (*(void **)*pCurrentFrame && nFrameCount < FRAME_LIMIT)
    {
	find((unsigned long)(*(pCurrentFrame + 1)), pFrame);
        pFrame += strlen(pFrame);
        nFrameCount++;
	if (((*(pCurrentFrame + 1) > pMakeProcess - 64)) &&
	    ((*(pCurrentFrame + 1) < pMakeProcess + 64)))
	    break;
        // make sure we aren't looking beyond the end of the stack.  we add
        // some slop due to find, above
	pCurrentFrame = *pCurrentFrame;
        if (*pCurrentFrame > (void*)(bos - 32))
            break;
    }

    sprintf(buffer, "\n%s\n\n", trace);
    CrashOutputWrapper(buffer);
}

char*
get_trace_from_context(ucontext_t* u)
{
    void** pCurrentFrame = (void**)u->uc_mcontext.gregs[REG_EBP];
    char trace[32768];
    char* pFrame;
    int nFrameCount = 0;
    char buffer[32780];

    find((unsigned long)u->uc_mcontext.gregs[REG_EIP], trace);
    pFrame = trace + strlen(trace);
    while (*(void **)*pCurrentFrame && nFrameCount < FRAME_LIMIT)
    {
	find((unsigned long)(*(pCurrentFrame + 1)), pFrame);
        pFrame += strlen(pFrame);
        nFrameCount++;
	if (((*(pCurrentFrame + 1) > pMakeProcess - 64)) &&
	    ((*(pCurrentFrame + 1) < pMakeProcess + 64)))
	    break;
	pCurrentFrame = *pCurrentFrame;
    }

    return trace;
}
#endif // _LINUX

char*
get_trace(void)
{
    void** pCurrentFrame = 0;
    static char trace[32768];
    int nFrameCount = 0;
    trace[0] = 0;

    __asm __volatile("movl %%ebp, %0" : "=d" (pCurrentFrame));

    while (*(void **)*pCurrentFrame && nFrameCount < FRAME_LIMIT)
    {
	find((unsigned long)(*(pCurrentFrame + 1)), trace + strlen(trace));
        nFrameCount++;

	if (((*(pCurrentFrame + 1) > pMakeProcess - 64)) &&
	    ((*(pCurrentFrame + 1) < pMakeProcess + 64)))
	    break;

	pCurrentFrame = *pCurrentFrame;
    }
   
    return trace;
}

void**
get_stack(int* size)
{
    void** pCurrentFrame = 0;                                      
    static void* stack[32768];                                   
    int nframes = 0;
    int frame = 0;

    __asm __volatile("movl %%ebp, %0" : "=d" (pCurrentFrame));
    while(*(void **)*pCurrentFrame)
    {
        if (((*(pCurrentFrame + 1) > pMakeProcess - 64)) &&
            ((*(pCurrentFrame + 1) < pMakeProcess + 64)))
            break;

	stack[frame++] = (*(pCurrentFrame + 1));
	pCurrentFrame = *pCurrentFrame;
    }
    *size = frame;                          
    return stack;
}

char*
get_trace_from_stack(void** stack, int size)
{
    int i;
    int at = 0;
    static char trace[32768];

    trace[0] = 0;

    for(i = 0; i < size; i++)
    {
	at += sprintf(&(trace[at]), "0x%x\n", stack[i]);
    }
    return trace;
}

#elif defined _SOLARIS

#include <dlfcn.h>
#include <stdio.h>
#include <strings.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/reg.h>
#include <sys/frame.h>
#include <sys/regset.h>
#include <ucontext.h>
#include <unistd.h>

#include <strings.h>
#include <setjmp.h>

char*
find(unsigned long addr)
{
    static char pName[2048];
    int i = 0;

    sprintf(pName, "0x%.8p\n", (void*)addr);
    return pName;
}

char*
get_trace_from_stack(void** stack, int size)
{
    int i;
    int at = 0;
    static char trace[32768];

    trace[0] = 0;

    for(i = 0; i < size; i++)
    {
	at += sprintf(&(trace[at]), "0x%.8p\n", stack[i]);
    }
    return trace;
}

/*
 * walks up call stack, printing library:routine+offset() for each routine
 *
 *
 * The output will have each frame something like:
 *
 *  pc: func(arg1, arg2, arg3, arg4, arg5, arg6) + offset
 *
 * Note that it does NOT correctly look up static functions
 */

#ifdef __sparcv9
#define FRAME_PTR_REGISTER         REG_SP
#define ARGUMENT_PTR(p)            ((long*)((p)->fr_arg))
#define BIAS                       2047

#elif defined(__sparc)
#define FRAME_PTR_REGISTER         REG_SP
#define ARGUMENT_PTR(p)            ((long*)((p)->fr_arg))
#define BIAS                       0

#elif defined(__i386)
#define FRAME_PTR_REGISTER         EBP
#define ARGUMENT_PTR(p)            (((long*)&((p)->fr_savfp)) + 2)
#define BIAS                       0
#endif


/*
 *  print_frame
 *
 *  As a first approximation, use "dladdr()" to search the
 *  dynamic loader symbol table.  Note that this will return
 *  the next-smaller non-static symbol when searching for
 *  functions declared "static" (which will not be in this
 *  symbol table).
 */
static void
print_frame(void *pc, long* args)
{
    Dl_info info;
    char *function, *library;

    if(dladdr(pc, &info) == 0) {
        function = "??";
        library = "??";
    }
    else {
        library =  (char*)info.dli_fname;
        function = (char*)info.dli_sname;
    }

    printf("0x%.8p: %s(0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx) + 0x%lx\n",
	    pc, function,
	    args[0], args[1], args[2], args[3], args[4], args[5],
	    (unsigned long)pc - (unsigned long)info.dli_saddr);

    return;
}


/*
 *  Walk the stack printing each calling function
 *
 *  For the call chain:         main() ==> one() ==> two() ==> tb()
 *
 *  The stack looks like:
 *    .--------------------------------------------------------------------.
 *    |  frame for tb  |  frame for two |  frame for one |  frame for main |
 *    `--------------------------------------------------------------------'
 *    ^                ^
 *    |                |
 *    SP               FP
 *
 *  Each frame looks like:
 *    .--------------------------------------------------------------------.
 *    | register save area | struct ret ptr[1]  | outgoing args | locals   |
 *    `--------------------------------------------------------------------'
 *       [1] Note: This field does not exist in the V9 ABI
 *  
 *  The register save area looks like:
 *    .--------------------------------------------------------------------.
 *    | local regs 0-7  | arg regs 0-5  | frame pointer  |  return address |
 *    `--------------------------------------------------------------------'
 */
char*
get_trace_from_context(ucontext_t* u)
{
    struct frame *curr_frame;
    void* function_address;
    Dl_info info;
    char *function, *library;
    long* args;
    static char trace[32767];
    static char one_frame[1024];
    int nFrameCount = 0;

    trace[0] = 0;
    one_frame[0] = 0;

    /*
     *  Initialize with the SP for this function.
     */
    curr_frame = 
	(struct frame*)((long)(u->uc_mcontext.gregs[FRAME_PTR_REGISTER]) + BIAS);
    function_address = (void*)(u->uc_mcontext.gregs[REG_PC]);

    /*
     *  Walk the stack and dump an "adb" style traceback along the way
     */
    do
    {
        if (!curr_frame->fr_savfp)
            break;

	args = ARGUMENT_PTR(curr_frame);

	if(dladdr(function_address, &info) == 0) {
            function = "??";
            library = "??";
	}
	else {
	    library =  (char*)info.dli_fname;
	    function = (char*)info.dli_sname;
	}

	sprintf(one_frame, 
		"0x%.8p: %s(0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx)"
		" + 0x%lx\n",
		function_address, function,
		args[0], args[1], args[2], args[3], args[4], args[5],
	       (unsigned long)function_address - (unsigned long)info.dli_saddr);

	strcat(trace, one_frame);
        nFrameCount++;
    } while ((function_address = (void*)(curr_frame->fr_savpc)) &&
             (curr_frame = (struct frame*)(long)(curr_frame->fr_savfp) + BIAS) &&
             nFrameCount < FRAME_LIMIT);

    return trace;
}

char* 
get_trace(void)
{
    /*  
     *  Since getcontext() is a syscall, it will flush the current register
     *  window on a SPARC, so the stack memory image will be up-to-date.
     */
    ucontext_t u;
    getcontext(&u);

    return get_trace_from_context(&u);
}

void**
get_stack(int* size)
{
    jmp_buf      jbuf;
    int frame = 0;
    struct frame *  sp;
    static void* stack[32768];

    /* machine specific trap number for flushing the register window.
     * may need to modified for other sparc types
     */
#ifdef __GNUC__
    __asm__ __volatile__("ta 3"); 
#else
    asm ("ta 3"); 
#endif
    _setjmp( jbuf );

    sp = (struct frame *) jbuf[1]; // solaris specific skip the first frame

    while( sp && sp->fr_savpc )
    {
        stack[frame++] = sp->fr_savpc;
	sp = (struct frame *) sp->fr_savfp;
    }

    *size = frame;
    return stack;
}

#elif defined _HPUX && !defined _IA64

#include <stdio.h>
#include <string.h>
#include "unwind.h"

extern void U_get_frame_info(cframe_info *);
extern void U_STACK_TRACE(void);
#ifndef __LP64__
static void copy_prev_to_curr(cframe_info *curr_frame,pframe_info *prev_frame);
#endif

void
get_ustack_trace()
{
    /* Prints to stderr. */
    U_STACK_TRACE();
}

char*
get_trace_from_stack(void** stack, int size)
{
    int i;
    int at = 0;
    static char trace[32768];

    trace[0] = 0;

    for(i = 0; i < size; i++)
    {
	at += sprintf(&(trace[at]), "0x%08x\n", stack[i]);
    }
    return trace;
}

char*
get_trace()
{
    static char stack_trace[65536];
    cframe_info curr_frame;
    pframe_info prev_frame;
    int depth, status;

    *stack_trace = '\0';
  
#ifdef __LP64__
    /* this is ok for 11.x and 32 bit */
    U_init_frame_record(&curr_frame);
    U_prep_frame_rec_for_unwind(&curr_frame);
#else
    /* set up a valid curr_frame by calling an assembly routine.
     * This assembly routine is not exported by HP, but can
     * be extracted from /usr/lib/libcl.a ... it is called
     * ugetfram.o. The U_get_frame_info routine MUST be put into 
     * the same image as this routine. It can then set up a dummy
     * curr_frame that has the correct values set.
     */
    U_get_frame_info(&curr_frame);
#endif


    /* throw away the first frame ... since its a dummy frame
     * created by the call to U_get_frame_info.
     */

    status = U_get_previous_frame_x(&curr_frame, &prev_frame, sizeof(cframe_info));

    /* Check to make sure everything is okay */

    if (status)
    {
      fprintf(stderr, "Stack_Trace: error while unwinding stack %d\n", status);
      return 0;
    }

    /* copy the prev_frame to the curr_frame */

#ifdef __LP64__
    U_copy_frame_info(&curr_frame, &prev_frame);
#else
    copy_prev_to_curr(&curr_frame, &prev_frame);
#endif

    /* Now for the real work. Initialize the trace string, and then
     * loop, unwinding a frame at a time until there are no more frames
     * to unwind (i.e. the offset portion of the return address is 0).
     */

    *stack_trace=0;
    for (depth = 0; curr_frame.currlo && depth < FRAME_LIMIT; depth++)
    {
        status = U_get_previous_frame_x(&curr_frame, &prev_frame, sizeof(cframe_info));

        /* Check to make sure everything is okay */

        if (status)
	{
	    fprintf(stderr,"Stack_Trace: error while unwinding stack depth=%d\n", depth);
	    fflush(stderr);
	    return 0;
	}

        /* Now, we'd like to print out the return pointer. However,
         * U_get_previous_frame returns the prev_frame for the 1st NON-STUB
         * frame in the call chain. It may be the case that the return
         * pointer for this frame points into another stub. What we'd 
         * really like to see is the return point for all NON-STUBS.
         * U_get_previous_frame updates curr_frame so that it contains
         * a frame whose return point is a NON-STUB. Print out this value
         * before copying over prev_frame into curr_frame.
         */

        sprintf(stack_trace + strlen(stack_trace),
	        "0x%l08x\n", curr_frame.currlo & ~3L);

#ifdef __LP64__
        U_copy_frame_info(&curr_frame, &prev_frame);
#else
        copy_prev_to_curr(&curr_frame, &prev_frame);
#endif
    }

    return stack_trace;
}

void**
get_stack(int* size)
{
    static char stack_trace[65536];
    cframe_info curr_frame;
    pframe_info prev_frame;
    int depth, status;
    void* stack[32768];
    int frame = 0;

    *stack_trace = '\0';
  
#ifdef __LP64__
    /* this is ok for 11.x and 32 bit */
    U_init_frame_record(&curr_frame);
    U_prep_frame_rec_for_unwind(&curr_frame);
#else
    /* set up a valid curr_frame by calling an assembly routine.
     * This assembly routine is not exported by HP, but can
     * be extracted from /usr/lib/libcl.a ... it is called
     * ugetfram.o. The U_get_frame_info routine MUST be put into 
     * the same image as this routine. It can then set up a dummy
     * curr_frame that has the correct values set.
     */
    U_get_frame_info(&curr_frame);
#endif


    /* throw away the first frame ... since its a dummy frame
     * created by the call to U_get_frame_info.
     */

    status = U_get_previous_frame_x(&curr_frame, &prev_frame, sizeof(cframe_info));

    /* Check to make sure everything is okay */

    if (status)
    {
      fprintf(stderr, "get_stack: error while unwinding stack %d\n", status);
      return 0;
    }

    /* copy the prev_frame to the curr_frame */

#ifdef __LP64__
    U_copy_frame_info(&curr_frame, &prev_frame);
#else
    copy_prev_to_curr(&curr_frame, &prev_frame);
#endif

    /* Now for the real work. Initialize the trace string, and then
     * loop, unwinding a frame at a time until there are no more frames
     * to unwind (i.e. the offset portion of the return address is 0).
     */

    *stack_trace=0;
    for (depth = 0; curr_frame.currlo && depth < FRAME_LIMIT; depth++)
    {
        status = U_get_previous_frame_x(&curr_frame, &prev_frame, sizeof(cframe_info));

        /* Check to make sure everything is okay */

        if (status)
	{
	    fprintf(stderr,"get_stack: error while unwinding stack depth=%d\n", depth);
	    fflush(stderr);
	    return 0;
	}

        /* Now, we'd like to print out the return pointer. However,
         * U_get_previous_frame returns the prev_frame for the 1st NON-STUB
         * frame in the call chain. It may be the case that the return
         * pointer for this frame points into another stub. What we'd 
         * really like to see is the return point for all NON-STUBS.
         * U_get_previous_frame updates curr_frame so that it contains
         * a frame whose return point is a NON-STUB. Print out this value
         * before copying over prev_frame into curr_frame.
         */

        stack[frame++] = curr_frame.currlo & ~3L;

#ifdef __LP64__
        U_copy_frame_info(&curr_frame, &prev_frame);
#else
        copy_prev_to_curr(&curr_frame, &prev_frame);
#endif
    }

    *size = frame;
    return stack;
}

#ifndef __LP64__
static void
copy_prev_to_curr(cframe_info *curr_frame, pframe_info *prev_frame)
{
    /* Update curr_frame with values returned in prev_frame */
    curr_frame->cur_frsize = prev_frame->prev_frsize;
    curr_frame->cursp = prev_frame->prevsp;
    curr_frame->currls = prev_frame->prevrls;
    curr_frame->currlo = prev_frame->prevrlo;
    curr_frame->curdp = prev_frame->prevdp;
    curr_frame->r3 = prev_frame->PFIinitR3;
    curr_frame->r4 = prev_frame->PFIinitR4;

    /* don't update curr_frame.cur_r19 because U_get_previous_frame does
     * it directly.
     */
}
#endif

#elif defined (_OSF1)
/* 
 * EDIT HISTORY:
 *
 * 
 * V1.01	whb		19-Mar-2001
 *    Hacked out the symbolic stack trace info and add in the __init_*
 * initialization routine to define the address of main() at library
 * image init time... Code's getting uglier, clean up needed big time!
 * Also note that static global context means this is will not work
 * with threads.
 *
 * V1.00	whb		16-Mar-2001
 *    Created hack for Atin to experiment with... No real review or testing!
 * It simply is a jacket routine on is_function_on_stack() and searches for
 * main()...
 *
 * 
 */

/*
 * This module contains is_function_on_stack and its support routines.
 * It's an example routine which takes an unsigned long which is presumed to 
 * be the address of an interesting function, and checks whether the 
 * function is on the current call stack.  
 *
 * Returns a "boolean" [long], 1 if the routine is on the stack, 0
 * otherwise.
 *
 * Compile with 
 *    cc [-g] is_routine_on_stack -lexc
 * for the UNIT_TEST version. Otherwise, remember to use -lexc on the
 * link of the program into which it is being included.
 *
 */
#include <excpt.h>

extern __start;
extern main;

unsigned long get_trace_base_addr;

static unsigned long sp;
static unsigned long pc;
static unsigned long prev_func;
static pdsc_rpd    * func_rpd;

extern long __exc_crd_list_head;

/*
 * Use an init routine to initialize the address of the main() routine.
 *
 */

void
__init_get_trace(void)
{
    // get_trace_base_addr = (unsigned long) &__start;
    get_trace_base_addr = (unsigned long) &main;

    // printf("get_trace_base_addr = 0x%lp\n", get_trace_base_addr);

}

static pdsc_rpd * 
get_primary_rpd (exc_address pc) 
{
    /*
     * As of Tru64 UNIX V5.1 there may be many "linked" RPDs, describing
     * a single procedure.  To detect whether two PCs fall in the same 
     * procedure, we must find the primary (distinguished) RPD for the
     * procedure.  This one will have a RETURN_ADDRESS field of zero.
     * Prior to V5.1, this linking was not used, and *ALL* RPDs had a
     * zero RETURN_ADDRESS field.
     */
    pdsc_rpd		* rpd;
    PRUNTIME_FUNCTION	crd;
    int			return_address;
    union pdsc_crd	* crd_base;

    return_address = 1;
    while (return_address) {
	crd = exc_remote_lookup_function_entry(
			       NULL, NULL, pc,
			       (exc_address)&__exc_crd_list_head,
			       &crd_base, NULL);
	rpd = PDSC_CRD_PRPD(crd);
	crd = 0;
	if (!rpd)
	    /*
	     * The routine is a null frame procedure, which doesn't
	     * have an RPD.  Our caller will have to know how to handle
	     * it. Note that a null frame procedure can't call anyone,
	     * therefore we can't get here.
	     */
	    return rpd;

#ifdef PDSC_RPD_RETURN_ADDRESS_FIELD
	return_address = PDSC_RPD_RETURN_ADDRESS_FIELD(rpd);
	if (return_address)
	    pc = PDSC_RPD_RETURN_ADDRESS(crd_base, rpd);
#else
	return_address = 0;
#endif
    }

    return(rpd);
}


/*
 * Determining end of the stack walk:
 * We have reached the end of the stack walk if either of two events happen,
 *  - the returned pc is 0, or
 *  - the stack pointer doesn't change and the pc doesn't change.
 */
static long 
done(PCONTEXT ctx) 
{
    if (ctx->sc_pc==0 || (ctx->sc_pc==pc && ctx->sc_regs[R_SP]==sp))
	return 1;
    pc = ctx->sc_pc;
    sp = ctx->sc_regs[R_SP];
    return 0;
}

/*
 * This is the main entry point for this file. Everything else is support!
 */
char * 
get_trace()
{
    static void* stack[32768];
    static char stack_trace[65536];
    struct sigcontext ctx;
    int nFrameCount = 0;

    *stack_trace = '\0';

    /*
     * Capture the current context (i.e., this routine's)
     */
    exc_capture_context(&ctx);

    /*
     * We can't really be done yet.  But we use the done function to 
     * initialize static variables.  Since we're calling it, abort if
     * it returns true.
     */
    if (done(&ctx))
    {
	sprintf(stack_trace + strlen(stack_trace),
	    "couldn't get entire call stack!\n");
	return stack_trace;
    }
  
    /*
     * Unwind once to get rid of the caller of this routine (get_trace).
     */
    exc_virtual_unwind(0, &ctx);

    sprintf(stack_trace + strlen(stack_trace), "0x%lp\n", 
	ctx.sc_pc);

    if (done(&ctx))
	return stack_trace;

    /*
     * Setup: Find the runtime procedure descriptor (RPD) for the
     * desired procedure.
     */
    if (prev_func != get_trace_base_addr || func_rpd == 0) {
	func_rpd = get_primary_rpd(get_trace_base_addr);

	/*
	 * Sanity: Make sure we got one! (Major foul-up somewhere!)
	 */
	if (func_rpd == 0)
	{
	    sprintf(stack_trace + strlen(stack_trace),
		"couldn't get stack frame!\n");
	    return stack_trace;
	}

	prev_func = get_trace_base_addr;
    }
  

    /*
     * Now walk the rest of the stack, and for each returned PC check
     * whether it is in the same routine as the desired PC.
     */
    
    while (nFrameCount < FRAME_LIMIT) {
	if (func_rpd == get_primary_rpd(pc))
	    break;

	exc_virtual_unwind(0, &ctx);

        nFrameCount++;
	sprintf(stack_trace + strlen(stack_trace), "0x%lp\n", 
	    ctx.sc_pc);

	if (done(&ctx))
	    break;
    }

    return stack_trace;
}


void**
get_stack(int* size) 
{
    static void* stack[32768];
    struct sigcontext ctx;
    int frame = 0;

    /*
     * Capture the current context (i.e., this routine's)
     */
    exc_capture_context(&ctx);

    /*
     * We can't really be done yet.  But we use the done function to 
     * initialize static variables.  Since we're calling it, abort if
     * it returns true.
     */
    if (done(&ctx))
    {
	stack[frame++] = 0;
	return stack;
    }
  
    /*
     * Unwind once to get rid of the caller of this routine (get_trace).
     */
    exc_virtual_unwind(0, &ctx);

    stack[frame++] = ctx.sc_pc;

    if (done(&ctx))
    {
	stack[frame++] = 0;
	return stack;
    }

    /*
     * Setup: Find the runtime procedure descriptor (RPD) for the
     * desired procedure.
     */
    if (prev_func != get_trace_base_addr || func_rpd == 0) {
	func_rpd = get_primary_rpd(get_trace_base_addr);

	/*
	 * Sanity: Make sure we got one! (Major foul-up somewhere!)
	 */
	if (func_rpd == 0)
	{
	    stack[frame++] = 0;
	    return stack;
	}

	prev_func = get_trace_base_addr;
    }
  

    /*
     * Now walk the rest of the stack, and for each returned PC check
     * whether it is in the same routine as the desired PC.
     */
    while (1) {    /* We'll return from within the loop. */
	if (func_rpd == get_primary_rpd(pc))
	    break;

	exc_virtual_unwind (0, &ctx);

	stack[frame++] = (void *)ctx.sc_pc;
	if (done(&ctx))
	{
	    break;
	}
    }
    *size = frame;
    return stack;
}

char*
get_trace_from_stack(void** stack, int size)
{
    int i;
    int at = 0;
    static char trace_from_stack[32768];

    trace_from_stack[0] = 0;

    for(i = 0; i < size; i++)
    {
	at += sprintf(&(trace_from_stack[at]), "0x%lx\n", stack[i]);
    }
    return trace_from_stack;
}


#elif defined _AIX

/*
 * AIX stack frames look like this:
 *
 *  offset
 *      .--------------------------.
 *    0 | ptr to prev. stack frame |
 *      +--------------------------+
 *    4 | ...                      |
 *      +--------------------------+
 *    8 | saved program counter[1] |
 *      +--------------------------+
 *   12 | ...                      |
 *      +--------------------------+
 *   16 | ...                      |
 *      +--------------------------+
 *   20 | ...                      |
 *      +--------------------------+
 *   24 | Arguments 1-8            |
 *      | (space for 8 32-bit args |
 *      | is always reserved)      |
 *      +--------------------------+
 *   56 |local variables           |
 *      +--------------------------+
 *      ...etc...
 *      +--------------------------+
 *   xx | register save area[2]    |
 *      `--------------------------'
 *
 *  
 *  [1]: set by prolog of callee routine, unless it doesn't call any
 *       other routines.
 *  [2]: This area is where the callee saves any registers it
 *       will be modifying so it can restore them when returning,
 * 
 *  General Purpose Register 1 contains the stack pointer.
 */
#include <stdio.h>
#include <ucontext.h>

extern char* HXGetStackFrame(); //implemented in servutil/unix/aix/stackframe.s

typedef struct stack_frame
{
    struct stack_frame *prev;
    int                 unused;
    int                 pc;
} stackFrame;

char*
get_trace_from_context(ucontext_t* u)
{
    static char stack_trace[65536];
    char* pCur;
    int nFrame;
    ucontext_t* pContext;
    int* pFunc;
    stackFrame* pStackFrame;

    pCur = stack_trace;
    pContext = u;
    stack_trace[0] = '\0';
    nFrame = 0;

    if (pContext)
    {
        pStackFrame = (stackFrame*)pContext->uc_mcontext.jmp_context.gpr[1];

        for (nFrame=0; pStackFrame && nFrame < FRAME_LIMIT; ++nFrame)
        {
            pFunc = (int*)(pStackFrame->pc);
            pCur += sprintf(pCur, "0x%08lx\n", pFunc);
            pStackFrame = pStackFrame->prev;
        }
    }

    return stack_trace;
}

char*
get_trace()
{
    ucontext_t u;
    getcontext(&u);
    return get_trace_from_context(&u);
}

void**
get_stack(int* size)
{
    static int* stack[32768];
    int nFrame=0;
    stackFrame* pStackFrame;

    pStackFrame = (stackFrame*)HXGetStackFrame();

    for (nFrame=0; pStackFrame && nFrame < FRAME_LIMIT; ++nFrame)
    {
        stack[nFrame] = (int*)(pStackFrame->pc);
        pStackFrame = pStackFrame->prev;
    }

    stack[nFrame] = 0;
    *size = nFrame;

    return (void**)&stack;
}

char*
get_trace_from_stack(void** stack, int size)
{
    int i;
    int at = 0;
    static char trace_from_stack[32768];

    trace_from_stack[0] = 0;

    for(i = 0; i < size; i++)
    {
	at += sprintf(&(trace_from_stack[at]), "0x%08lx\n", stack[i]);
    }
    return trace_from_stack;
}

char*
find(unsigned long addr)
{
    static char pName[2048];
    int i = 0;

    sprintf(pName, "0x%08lx\n", (int*)addr);
    return pName;
}

void
setup_trace(char *prog)
{
}


#elif defined _WIN32

#include <windows.h>
#include <stdio.h>

__declspec(thread) char g_threadLocal_trace[32768];
__declspec(thread) char g_threadLocal_pName[2048];
__declspec(thread) void* g_threadLocal_stack[32768];

char*
find(unsigned long addr)
{
    int i = 0;

    sprintf(g_threadLocal_pName, "0x%p\n", (void*)addr);
    return g_threadLocal_pName;
}

char*
get_trace()
{
    DWORD *pFrame, *pPrevFrame;
    int nFrameCount = 0;
    g_threadLocal_trace[0] = 0;

    _asm
    {
	mov eax, ebp
	mov pFrame, eax
    }

    while(nFrameCount < FRAME_LIMIT)
    {
        char* pCurrent;
	strcat(g_threadLocal_trace, pCurrent = find((*(pFrame + 1))));

        nFrameCount++;

	/* 
	 * If this is the first one, then ignore, since it will be the addr of
	 * printf_stack.
	 */

	pPrevFrame = pFrame;

	pFrame = (DWORD*)pFrame[0];

	if((DWORD)pFrame & 3) //Is frame pointer aligned on DWORD boundry?
	{
	    break;
	}

	if(pFrame <= pPrevFrame)
	{
	    break;
	}

	if(IsBadWritePtr(pFrame, sizeof(void*)*2))
	{
	    break;
	}

    }
    return g_threadLocal_trace;
}

void**
get_stack(int* size)
{
    DWORD *pFrame, *pPrevFrame;
    int nFrameCount = 0;

    _asm
    {
	mov eax, ebp
	mov pFrame, eax
    }

    while(nFrameCount < FRAME_LIMIT)
    {
        g_threadLocal_stack[nFrameCount++] = (void*)(*(pFrame + 1));
	/* 
	 * If this is the first one, then ignore, since it will be the addr of
	 * printf_stack.
	 */

	pPrevFrame = pFrame;

	pFrame = (DWORD*)pFrame[0];

	if((DWORD)pFrame & 3) //Is frame pointer aligned on DWORD boundry?
	{
	    break;
	}

	if(pFrame <= pPrevFrame)
	{
	    break;
	}

	if(IsBadWritePtr(pFrame, sizeof(void*)*2))
	{
	    break;
	}

    }
    *size = nFrameCount;

    return g_threadLocal_stack;
}

char*
get_trace_from_stack(void** stack, int size)
{
    int i;
    int at = 0;

    g_threadLocal_trace[0] = 0;

    for(i = 0; i < size; i++)
    {
	at += sprintf(&(g_threadLocal_trace[at]), "0x%x\n", stack[i]);
    }
    return g_threadLocal_trace;
}

void
setup_trace(char *prog)
{
}

#else

char*
get_trace()
{
    return "";
}

void**
get_stack(int* size)
{
    return 0;
}

char*
get_trace_from_stack(void** stack, int size)
{
    return "";
}

char*
find(unsigned long addr)
{
    return 0;
}

void
setup_trace(char *prog)
{
}

#if defined _LINUX && defined __amd64__
#include <ucontext.h>
char*
get_trace_from_context(ucontext_t* u)
{
    //use a stub for Linux/AMD64 for now...
    return "";
}
#endif


#endif
