/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: nm.c,v 1.4 2003/08/17 14:41:46 dcollins Exp $ 
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
 *  Symbol table information extraction routines.
 *
 */


/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Hans Huebner.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(__GNUC__) && defined(_DEBUG) && \
    (defined(i386) || defined(__i386__)) && \
    !defined(_LINUX)

#include <sys/types.h>
#include <a.out.h>
#include <stab.h>
#include <ar.h>
#include <dirent.h>
#include <ranlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nm.h"

int
setup_stab(char *objname, FILE *fp, struct alist **line_list, int *linelistlen);

/* some macros for symbol type (nlist.n_type) handling */
#define	SYMBOL_TYPE(x)		((x) & (N_TYPE | N_STAB))

int
setup_stab(objname, fp, line_list, linelistlen)
	char *objname;
	FILE *fp;
	struct alist **line_list;
	int *linelistlen;
{
	register struct nlist *names, *np;
	register int i, j, nnames, nrawnames;
	struct exec head;
	long stabsize;
	char *stab;
	struct alist *fun_list;
	int line_count;
	int fun_count;
	char *current_file;
	caddr_t current, next;
	int line_search_start;

	/* read a.out header */
	if (fread((char *)&head, sizeof(head), (size_t)1, fp) != 1) {
		(void)fprintf(stderr,
		    "nm: %s: cannot read header.\n", objname);
		return(1);
	}

	/*
	 * skip back to the header - the N_-macros return values relative
	 * to the beginning of the a.out header
	 */
	if (fseek(fp, (long)-sizeof(head), SEEK_CUR)) {
		(void)fprintf(stderr,
		    "nm: %s: %s\n", objname, strerror(errno));
		return(1);
	}

	/* stop if this is no valid object file */
	if (N_BADMAG(head)) {
		(void)fprintf(stderr,
		    "nm: %s: bad format.\n", objname);
		return(1);
	}

	/* stop if the object file contains no symbol table */
	if (!head.a_syms) {
		(void)fprintf(stderr,
		    "nm: %s: no name list.\n", objname);
		return(1);
	}

	if (fseek(fp, (long)N_SYMOFF(head), SEEK_CUR)) {
		(void)fprintf(stderr,
		    "nm: %s: %s\n", objname, strerror(errno));
		return(1);
	}

	/* get memory for the symbol table */
	names = malloc((size_t)head.a_syms);
	nrawnames = head.a_syms / sizeof(*names);
	if (fread((char *)names, (size_t)head.a_syms, (size_t)1, fp) != 1) {
		(void)fprintf(stderr,
		    "nm: %s: cannot read symbol table.\n", objname);
		(void)free((char *)names);
		return(1);
	}

	/*
	 * Following the symbol table comes the string table.  The first
	 * 4-byte-integer gives the total size of the string table
	 * _including_ the size specification itself.
	 */
	if (fread((char *)&stabsize, sizeof(stabsize), (size_t)1, fp) != 1) {
		(void)fprintf(stderr,
		    "nm: %s: cannot read stab size.\n", objname);
		(void)free((char *)names);
		return(1);
	}
	stab = malloc((size_t)stabsize);

	/*
	 * read the string table offset by 4 - all indices into the string
	 * table include the size specification.
	 */
	stabsize -= 4;		/* we already have the size */
	if (fread(stab + 4, (size_t)stabsize, (size_t)1, fp) != 1) {
		(void)fprintf(stderr,
		    "nm: %s: stab truncated..\n", objname);
		(void)free((char *)names);
		(void)free(stab);
		return(1);
	}

	/*
	 * fix up the symbol table and filter out unwanted entries
	 *
	 * common symbols are characterized by a n_type of N_UNDF and a
	 * non-zero n_value -- change n_type to N_COMM for all such
	 * symbols to make life easier later.
	 *
	 * filter out all entries which we don't want to print anyway
	 */
	for (np = names, i = nnames = 0; i < nrawnames; np++, i++) {
		if (SYMBOL_TYPE(np->n_type) == N_UNDF && np->n_value)
			np->n_type = N_COMM | (np->n_type & N_EXT);

		/*
		 * make n_un.n_name a character pointer by adding the string
		 * table's base to n_un.n_strx
		 *
		 * don't mess with zero offsets
		 */
		if (np->n_un.n_strx)
			np->n_un.n_name = stab + np->n_un.n_strx;
		else
			np->n_un.n_name = "";
		names[nnames++] = *np;
	}

	/* Round 1: Count FUN/SLINE types and allocate memory */
	fun_count = 1;
	line_count = 1;
	for (np = names, i = 0; i < nnames; np++, i++)
	{
		if (np->n_type == N_FUN)
		    fun_count++;
		if (np->n_type == N_SLINE)
		    line_count++;
	}

	fun_list = malloc(fun_count * sizeof(struct alist));
	*line_list = malloc(line_count * sizeof(struct alist));

	/* Round 2: Record FUN and SLINE references */
	fun_count = 0;
	line_count = 0;
	current_file = "";
	/* print out symbols */
	for (np = names, i = 0; i < nnames; np++, i++)
	{
		if ((np->n_type == N_SO) || (np->n_type == N_SOL))
		{
		    current_file = np->n_un.n_name;
		}
		if (np->n_type == N_FUN)
		{
		    (fun_list)[fun_count].filename = current_file;
		    (fun_list)[fun_count].addr     = (caddr_t)np->n_value;
		    (fun_list)[fun_count].entry    = np;
		    fun_count++;
		}
		if (np->n_type == N_SLINE)
		{
		    (*line_list)[line_count].filename = "";
		    (*line_list)[line_count].addr     = (caddr_t)np->n_value;
		    (*line_list)[line_count].entry    = np;
		    line_count++;
		}
	}

	*linelistlen = line_count;
	memset (&((fun_list)[fun_count]), 0, sizeof (struct alist));
	memset (&((*line_list)[line_count]), 0, sizeof (struct alist));

	/* Round 3: Associate SLINE references to FUN references */
	current_file = "";
	line_search_start = 0;
	/* print out symbols */
	for (i = 0; i < fun_count; i++)
	{
	    	current = (fun_list)[i].addr;
	    	next    = (fun_list)[i + 1].addr;
		if (!next)
		    next = (caddr_t)0xffffffff;
		for (j = line_search_start; j < line_count; j++)
		{
		    if ((*line_list)[j].addr >= next)
		    {
			line_search_start = j;
			break;
		    }
		    else
		    {
			(*line_list)[j].filename = (fun_list)[i].filename;
			(*line_list)[j].line     = (*line_list)[j].entry->n_desc;
			(*line_list)[j].entry    = (fun_list)[i].entry;
		    }
		}
	}

	(void)free(stab);
	return(0);
}
#endif
