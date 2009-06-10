/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: winff.cpp,v 1.9 2007/07/06 20:35:17 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hlxclib/windows.h"
#include <stdlib.h>
#include <string.h>

#if !defined(WIN32_PLATFORM_PSPC)
#include <dos.h>
#endif /* !defined(WIN32_PLATFORM_PSPC) */

#include "hlxclib/io.h"
#include <ctype.h>

#include "hxresult.h"
#include "hxstrutl.h"
#include "findfile.h"
#include "platform/win/winff.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


CWinFindFile::CWinFindFile  (   const char *path,
			    const char *delimiter,
			    const char *pattern) :
    CFindFile (path, delimiter, pattern)
{
#ifdef _WIN32
    m_lFileHandle	    = 0;
    m_bDone		    = FALSE;
#endif
    m_pCurrentDirectory	    = NULL;
    m_pCurrentFileName	    = new char[_MAX_PATH];
    m_pNextFileName	    = new char[_MAX_PATH];

    memset(m_pCurrentFileName, 0, _MAX_PATH);
    memset(m_pNextFileName, 0, _MAX_PATH);
}

CWinFindFile::~CWinFindFile()
{
    if (m_pCurrentDirectory)
    {
	delete [] m_pCurrentDirectory;
	m_pCurrentDirectory = 0;
    }			

    if (m_pCurrentFileName)
    {
	delete [] m_pCurrentFileName;
	m_pCurrentFileName = 0;
    }

    if (m_pNextFileName)
    {
	delete [] m_pNextFileName;
	m_pNextFileName = 0;
    }

#ifndef _WIN16
    if (m_lFileHandle)
    {
        OS_CloseDirectory();
    }
#endif
}

//
// Open the directory; initialize the directory handle.
// Return FALSE if the directory couldn't be opened.
//
HXBOOL CWinFindFile::OS_OpenDirectory (const char *dirname)
{
#ifdef _WIN32
    /* don't call this before calling CloseDirectory()!*/
    if (m_lFileHandle > 0)
	return FALSE;
#endif

    if (m_pCurrentDirectory)
    {
	delete [] m_pCurrentDirectory;
	m_pCurrentDirectory = 0;
    }

    UINT16 ulPatLen = 0;
    if (m_pattern)
    {
	ulPatLen = strlen(m_pattern);
    }

    /* 1 for \ and 1 for NULL at end */
    UINT32 ulLen = strlen(dirname) + ulPatLen + 2;
    m_pCurrentDirectory = new char[ulLen];
    if (!m_pCurrentDirectory)
	return FALSE;

    SafeStrCpy(m_pCurrentDirectory, dirname, ulLen);

    if (m_pattern)
    {
	if(strlen(m_pCurrentDirectory) &&
	   m_pCurrentDirectory[strlen(m_pCurrentDirectory) - 1] != '\\')
	SafeStrCat(m_pCurrentDirectory, OS_PATH_DELIMITER, ulLen);
	SafeStrCat(m_pCurrentDirectory, m_pattern, ulLen);
    }

#ifdef _WIN32
#if !defined(WIN32_PLATFORM_PSPC)
    m_lFileHandle = _findfirst( m_pCurrentDirectory, &m_FileInfo);
#else /* !defined(WIN32_PLATFORM_PSPC) */
    m_lFileHandle = FindFirstFile( OS_STRING(m_pCurrentDirectory), &m_FileInfo);
#endif /* !defined(WIN32_PLATFORM_PSPC) */

    if (m_lFileHandle > 0)
    {
	if (!m_pNextFileName)
	{
	    m_pNextFileName	    = new char[_MAX_PATH];
	    if (!m_pNextFileName)
	    {
		return FALSE;
	    }
	}

	if (!m_pCurrentFileName)
	{
	    m_pCurrentFileName	    = new char[_MAX_PATH];
	    if (!m_pCurrentFileName)
	    {
		return FALSE;
	    }
	}

#ifndef WIN32_PLATFORM_PSPC
	SafeStrCpy(m_pNextFileName, m_FileInfo.name, _MAX_PATH);
#else
	SafeStrCpy(m_pNextFileName, OS_STRING(m_FileInfo.cFileName), _MAX_PATH);
#endif
	return TRUE;
    }
    else
    {
	return FALSE;
    }
#else
    if (!_dos_findfirst( m_pCurrentDirectory, _A_ARCH | _A_HIDDEN | _A_NORMAL | _A_RDONLY, &m_FileInfo))
    {
	SafeStrCpy(m_pNextFileName, m_FileInfo.name, _MAX_PATH);
	return TRUE;
    }
    else
    {
	return FALSE;
    }
#endif // _WIN32
}

//
// Get the next file in the directory.  This *does not*
// filter out based on the pattern; every file is returned.
//
char* CWinFindFile::OS_GetNextFile()
{
    if (m_pNextFileName)
    {
	SafeStrCpy(m_pCurrentFileName, m_pNextFileName, _MAX_PATH);
    }
    else
    {
	if (m_pCurrentFileName)
	{
	    delete [] m_pCurrentFileName;
	    m_pCurrentFileName = NULL;
	}
    }

    if (m_pNextFileName)
    {
#if defined(_WIN32) && !defined(WIN32_PLATFORM_PSPC)
	if (_findnext( m_lFileHandle,&m_FileInfo) !=-1 )
#elif defined(WIN32_PLATFORM_PSPC)
	if (FindNextFile( m_lFileHandle,&m_FileInfo) != 0 )
#else
    	if (_dos_findnext( &m_FileInfo) == 0)
#endif
	{
#ifndef WIN32_PLATFORM_PSPC
	SafeStrCpy(m_pNextFileName, m_FileInfo.name, _MAX_PATH);
#else
	SafeStrCpy(m_pNextFileName, OS_STRING(m_FileInfo.cFileName), _MAX_PATH);
#endif
	}
	else
	{
	    delete [] m_pNextFileName;
	    m_pNextFileName = 0;
	}
    }

    return m_pCurrentFileName;
}

//
// release the directory
//
void CWinFindFile::OS_CloseDirectory ()
{
    if( m_lFileHandle )
    {
#if defined(_WIN32) && !defined(WIN32_PLATFORM_PSPC)
	_findclose(m_lFileHandle);
#elif defined(WIN32_PLATFORM_PSPC) && !defined(_WIN32_WCE_EMULATION)
	FindClose(m_lFileHandle);
#endif

	m_lFileHandle = 0;
    }
}

HXBOOL CWinFindFile::OS_InitPattern ()
{
    return TRUE;
}

HXBOOL CWinFindFile::OS_FileMatchesPattern (const char * fname)
{
    /* All the files that we get are already pattern matched */
    return TRUE;
}

void CWinFindFile::OS_FreePattern ()
{
    return;
}


