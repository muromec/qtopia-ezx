/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: findfile.cpp,v 1.14 2008/02/05 06:06:08 vkathuria Exp $
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

#include "findfile.h"

#if defined (_WINDOWS) || defined (_WIN32)

#if !defined(WIN32_PLATFORM_PSPC)
#include <dos.h>
#endif /* !defined(WIN32_PLATFORM_PSPC) */

#include "hlxclib/io.h"
#include "platform/win/winff.h"
#elif defined (_UNIX)
#include "platform/unix/unixff.h"
#elif defined (_SYMBIAN)
#include "platform/symbian/symbianff.h"
#elif defined (_OPENWAVE)
#include "platform/openwave/opwaveff.h"
#elif defined (_MACINTOSH)
#include "platform/mac/macff.h"
#endif

#ifdef _UNIX
#include <stdlib.h>
#endif

#include "hlxclib/string.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif



CFindFile*
CFindFile::CreateFindFile(   const char *path,
			    const char *delimiter,
			    const char *pattern,
			    IUnknown** ppCommonObj)
{
    CFindFile* pFindFile = NULL;
#if defined (_WINDOWS) || defined (_WIN32)
    pFindFile	= new CWinFindFile(path, delimiter, pattern);
#elif defined (_UNIX)
    pFindFile	= new CUnixFindFile(path, delimiter, pattern);
#elif defined (_MACINTOSH)
    pFindFile	= new CMacFindFile(path, delimiter, pattern);
#elif defined (_SYMBIAN)
    pFindFile	= new CSymbianFindFile(path, delimiter, pattern, ppCommonObj);
#endif

    return pFindFile;
}

CFindFile::CFindFile	(   const char *path,
			    const char *delimiter,
			    const char *pattern)
{
    m_searchPathDelimiter = NULL;
    m_pattern = NULL;
    m_curFilename = NULL;
    m_pFilePath	= NULL;

    if (path == NULL)
	return;

    m_searchPath = path;

    if (delimiter)			      
    {
	m_searchPathDelimiter = new char [ strlen (delimiter) + 1 ];
	strcpy (m_searchPathDelimiter, delimiter); /* Flawfinder: ignore */
    }

    if (pattern)
    {
	m_pattern = new char [ strlen (pattern) + 1 ];
	strcpy (m_pattern, pattern); /* Flawfinder: ignore */
    }

    m_curDir = NULL;
    m_started = FALSE;
}

CFindFile::~CFindFile()
{
    if (m_searchPathDelimiter)
	delete [] m_searchPathDelimiter;
    if (m_pattern)
	delete [] m_pattern;
    if (m_curFilename)
	delete [] m_curFilename;
    if (m_pFilePath)
	delete [] m_pFilePath;
}

char *CFindFile::FindFirst ()
{
    HXBOOL foundFirstDir = FALSE;

    if (OS_InitPattern () == FALSE)
    {
	return NULL;
    }

    if (m_searchPathDelimiter)
    {
	m_curDir = strtok ((char*) ((const char*) m_searchPath), m_searchPathDelimiter);
    }
    else
    {
	m_curDir = (char*) ((const char*) m_searchPath);
    }

    while (!foundFirstDir && m_curDir != NULL)
    {
	if (OS_OpenDirectory (m_curDir) == TRUE)
	{
	    foundFirstDir = TRUE;
	}
	else
	{
	    if (m_searchPathDelimiter)
	    {
		m_curDir = strtok (NULL, m_searchPathDelimiter);
	    }
	    else
	    {
		m_curDir = NULL;
	    }
	}
    }

    if (foundFirstDir)
    {
	m_started = TRUE;
	return (FindNext ());
    }
    else
    {
	return NULL;
    }
}

char *CFindFile::FindNext ()
{
    if (!m_started)
    {
	return NULL;
    }

    char *fname = NULL;
    HXBOOL doneWithDirs = FALSE;

    if (m_curFilename != NULL)
    {
	delete [] m_curFilename;
	m_curFilename = NULL;
    }

    // keep going until the file is found or we're out of
    // directories to search for it in.
    while (m_curFilename == NULL && doneWithDirs == FALSE)
    {
	fname = OS_GetNextFile();
	// if we've got a file, see if it's a match
	if (fname != NULL)
	{
	    if (OS_FileMatchesPattern (fname))
	    {
		m_curFilename = new char [ strlen (fname) + 1 ];
		strcpy (m_curFilename, fname); /* Flawfinder: ignore */
	    }
	}
	// otherwise go on to the next directory
	else
	{
	    if (m_searchPathDelimiter)
	    {
		m_curDir = strtok (NULL, m_searchPathDelimiter);
	    }
	    else
	    {
		m_curDir = NULL;
	    }

	    if (m_curDir != NULL)
	    {
		// don't care if this fails; OS_GetNextFile() will
		// return null in that case
		OS_CloseDirectory ();
		doneWithDirs = (OS_OpenDirectory (m_curDir) == FALSE);
	    }
	    else
	    {
                OS_CloseDirectory ();
		// were're out of tokens, so we're done
		doneWithDirs = TRUE;
	    }
	}
    }

    if (m_curFilename)
    {
	if (m_pFilePath)
	{
	    delete [] m_pFilePath;
	    m_pFilePath = 0;
	}

	m_pFilePath = new char[	strlen(m_curFilename) + strlen(m_curDir) + 
				strlen(OS_PATH_DELIMITER) + 1];
	if (!m_pFilePath)
	    return NULL;

	strcpy(m_pFilePath, m_curDir); /* Flawfinder: ignore */
	if (memcmp(&m_curDir[strlen(m_curDir) - 1], OS_PATH_DELIMITER, 1))
	{
	    strcat(m_pFilePath, OS_PATH_DELIMITER); /* Flawfinder: ignore */
	}
	strcat(m_pFilePath, m_curFilename); /* Flawfinder: ignore */
    }

    return (m_curFilename);
}


