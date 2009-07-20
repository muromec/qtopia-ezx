/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: unixff.cpp,v 1.5 2007/07/06 20:35:16 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer and/or licensor of the Original Code and owns the
 * copyrights in the portions it created.
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

/* Parts of pmatch() are copyright: */

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
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
 *
 */


#include <stdlib.h>
#include <string.h>

#include "findfile.h"
#include "platform/unix/unixff.h"

CUnixFindFile::CUnixFindFile (   const char *path,
				const char *delimiter,
				const char *pattern) : 
    CFindFile (path, delimiter, pattern)
{
    // initialize platform-specific variables 
    m_dirHandle = NULL;
    m_dirEntry = NULL;
}


CUnixFindFile::~CUnixFindFile()
{
    if (m_dirHandle != NULL)
    {
        OS_CloseDirectory();
    }
}

//
// Open the directory; initialize the directory handle.
// Return FALSE if the directory couldn't be opened.
//
HXBOOL CUnixFindFile::OS_OpenDirectory (const char *dirname)
{
    // don't call this before calling CloseDirectory()!
    if (m_dirHandle != NULL)
	return FALSE;

    m_dirHandle = opendir (dirname);
    return (m_dirHandle == NULL ? FALSE:TRUE);
}

//
// Get the next file in the directory.  This *does not*
// filter out based on the pattern; every file is returned.
//
char* CUnixFindFile::OS_GetNextFile()
{
    if (!m_dirHandle)
	return NULL;

    m_dirEntry = readdir (m_dirHandle);
    if (m_dirEntry != NULL)
	return (m_dirEntry->d_name);
    else
	return NULL;
}

//
// release the directory
//
void CUnixFindFile::OS_CloseDirectory ()
{
    if (m_dirHandle)
	closedir (m_dirHandle);

    m_dirHandle = NULL;
}

HXBOOL CUnixFindFile::OS_InitPattern ()
{
    return (TRUE);
}

static int
pmatch(const char* pattern, const char* string)
{
    const char *p, *q;
    char c;

    p = pattern;
    q = string;
    for (;;) {
	switch (c = *p++) {
	case '\0':
	    goto breakloop;
	case '?':
	    if (*q++ == '\0')
		return 0;
	break;
	case '*':
	    c = *p;
	    if (c != '?' && c != '*' && c != '[') {
		while (*q != c) {
		    if (*q == '\0')
			return 0;
		    q++;
		}
	    }
	    do {
		if (pmatch(p, q))
		    return 1;
	    } while (*q++ != '\0');
	return 0;
	case '[': {
		const char *endp;
		int invert, found;
		char chr;

		endp = p;
		if (*endp == '!')
			endp++;
		for (;;) {
			if (*endp == '\0')
				goto dft;		/* no matching ] */
			if (*++endp == ']')
				break;
		}
		invert = 0;
		if (*p == '!') {
			invert++;
			p++;
		}
		found = 0;
		chr = *q++;
		if (chr == '\0')
			return 0;
		c = *p++;
		do {
		    if (*p == '-' && p[1] != ']') {
			p++;
#if 0
			if (   collate_range_cmp(chr, c) >= 0
			    && collate_range_cmp(chr, *p) <= 0
			   )
			    found = 1;
#endif
			p++;
		    } else {
			if (chr == c)
			    found = 1;
		    }
		} while ((c = *p++) != ']');
		if (found == invert)
		    return 0;
		break;
	    }
	    dft:
	    default:
		if (*q++ != c)
		    return 0;
	    break;
	}
    }
breakloop:
    if (*q != '\0')
	return 0;
    return 1;
}

HXBOOL CUnixFindFile::OS_FileMatchesPattern (const char * fname)
{
    return pmatch(m_pattern, fname);
}

void CUnixFindFile::OS_FreePattern ()
{
}


