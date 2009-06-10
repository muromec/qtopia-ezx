/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: allocdump.h,v 1.2 2003/01/23 23:42:54 damonlan Exp $ 
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

#include <time.h>


#define TIME_ALLOCS 100

#ifndef PAULM_DUMPALLOCS_HERE
extern FILE* g_fp_alloc;
extern int g_allocnum;
extern int g_freenum;
extern BOOL m_bAllocDump;
#endif


inline void
checktime()
{
    if ((g_allocnum + g_freenum) % TIME_ALLOCS == 0)
    {
	time_t tm;
	time(&tm);
	unsigned char type = 1;
	fwrite(&type, 1, 1, g_fp_alloc);
        fwrite(&tm, sizeof(time_t), 1, g_fp_alloc);
	fflush(g_fp_alloc);
    }
}

inline void
flush_alloc()
{
    if (g_fp_alloc)
    {
	checktime();
	fflush(g_fp_alloc);
    }
}

inline int
init_track_alloc()
{
    if (!g_fp_alloc)
    {
	g_fp_alloc = fopen("allocdump", "wb");
	if (g_fp_alloc)
	{
	    checktime();
	}
    }

    return (int)g_fp_alloc;
}

inline void
track_alloc(int size, void* addr)
{
    if (!init_track_alloc())
	return;

    int stack_size;
    void** pStack;
    pStack = get_stack(&stack_size);
    g_allocnum++;
    unsigned char type = 2;
    fwrite(&type, 1, 1, g_fp_alloc);
    fwrite(&g_allocnum, 4, 1, g_fp_alloc);
    fwrite(&addr, 4, 1, g_fp_alloc);
    fwrite(&size, 4, 1, g_fp_alloc);
    fwrite(&stack_size, 4, 1, g_fp_alloc);
    fwrite(pStack, 4, stack_size, g_fp_alloc);
    checktime();
}

inline void
track_free(char* addr)
{
    if (!init_track_alloc())
	return;

    int stack_size;
    void** pStack;
    pStack = get_stack(&stack_size);
    g_freenum++;
    unsigned char type = 3;
    fwrite(&type, 1, 1, g_fp_alloc);
    fwrite(&g_freenum, 4, 1, g_fp_alloc);
    fwrite(&addr, 4, 1, g_fp_alloc);
    fwrite(&stack_size, 4, 1, g_fp_alloc);
    fwrite(pStack, 4, stack_size, g_fp_alloc);
    checktime();
}
