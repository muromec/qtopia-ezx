/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdir.cpp,v 1.10 2009/01/19 23:30:46 sfu Exp $
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

#include <sys/stat.h>
#include <errno.h>

#if defined (_SOLARIS) || defined (_FREEBSD) || defined (_OPENBSD) || defined (_NETBSD) || defined (ANDROID)
#include <dirent.h>
#elif defined (__hpux)
#include <sys/dirent.h>
#else
#include <sys/dir.h>
#endif
#ifdef _BEOS
#include <fcntl.h>
#endif

#include "findfile.h"
#include "hxdir.h"
#include "hxstrutl.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

CHXDirectory::CHXDirectory()
    : m_pFileFinder(NULL)
{
}

CHXDirectory::~CHXDirectory()
{
    HX_DELETE(m_pFileFinder);
}

// Locate a writable spot for a temp directory.
// This is done by checking a number of possible locations
// in the following order of preference:
//
// 1) the current working directory
// 2) "/tmp"

static HXBOOL
isWritable(const char* szPath)
{
    struct stat statbuf;

    stat(szPath, &statbuf);
    if ((statbuf.st_mode & S_IWRITE) != 0)
        return TRUE;
    else
	return FALSE;
}

HXBOOL
CHXDirectory::SetTempPath(HXXHANDLE /* hpsHandle */, const char* szRelPath)
{
    // caller must specify a sub-directory
    if (szRelPath == NULL || szRelPath[0] == '\0') return FALSE;

    m_strPath.Empty();

    // try current working directory
    if (!SetCurrentDir() || !isWritable(m_strPath))
    {
	// go with /tmp
	m_strPath = "/tmp";
	if (!isWritable(m_strPath)) return FALSE;
    }
    
    // now append the sub-directory, separating if necessary
    if (m_strPath.Right(1) != OS_SEPARATOR_STRING && szRelPath[0] != OS_SEPARATOR_CHAR)
    {
	m_strPath += OS_SEPARATOR_STRING;
    }
    m_strPath += szRelPath;

    return TRUE;
}

/* Creates directory. */
HXBOOL 
CHXDirectory::Create()
{
    mkdir((const char*)m_strPath, 0755);
    return IsValid();
}

/* Checks if directory exists. */    
HXBOOL 
CHXDirectory::IsValid()
{
    DIR* pDir = NULL;

    if(m_strPath.IsEmpty())
        return FALSE;

    pDir = opendir(m_strPath);
    if (pDir)
    {
	closedir(pDir);
	return TRUE;
    }

    return FALSE;
}

/* Deletes empty directory. */
HXBOOL 
CHXDirectory::DeleteDirectory()
{
    if(!rmdir(m_strPath))
        return TRUE;
    return FALSE;
}

/* Starts enumeration process. */
CHXDirectory::FSOBJ 
CHXDirectory::FindFirst(const char* szPattern, char* szPath, UINT16 nSize)
{
    FSOBJ RetVal = FSOBJ_NOTVALID;
    char* szMatch = NULL;
    char* szMatchPath = NULL;
    HXBOOL  bDone = FALSE;
    struct stat statbuf;

    // Find the first file that matches the specified pattern
    HX_DELETE(m_pFileFinder); 
    m_pFileFinder = CFindFile::CreateFindFile(m_strPath, 0, szPattern);
    if (!m_pFileFinder)
    {
	return RetVal;	
    }

    szMatch = m_pFileFinder->FindFirst();
    while (szMatch && !bDone)
    {
	szMatchPath = m_pFileFinder->GetCurFilePath();

	if (lstat(szMatchPath, &statbuf) < 0)
	{
	    return RetVal;
	}

	if (S_ISDIR(statbuf.st_mode) != 0 && IsValidFileDirName(szMatch))
	{
	    RetVal = FSOBJ_DIRECTORY;
	    bDone = TRUE;
	}
	else if (IsValidFileDirName(szMatch))
	{
	    RetVal = FSOBJ_FILE;
	    bDone = TRUE;
	}
	else
	{
	    // If we couldn't use this one, find another
	    szMatch = m_pFileFinder->FindNext();
	}

	if (RetVal != FSOBJ_NOTVALID)
	{
	    SafeStrCpy(szPath, szMatchPath, nSize);
	}
    }

    return RetVal;
}

/* Continues enumeration process. */
CHXDirectory::FSOBJ 
CHXDirectory::FindNext(char* szPath, UINT16 nSize)
{
    FSOBJ RetVal = FSOBJ_NOTVALID;
    char* szMatch = NULL;
    char* szMatchPath = NULL;
    HXBOOL  bDone = FALSE;
    struct stat statbuf;
    
    szMatch = m_pFileFinder->FindNext();
    while (szMatch && !bDone)
    {
	szMatchPath = m_pFileFinder->GetCurFilePath();
	
	if (lstat(szMatchPath, &statbuf) < 0)
	{
	    return RetVal;
	}

	if (S_ISDIR(statbuf.st_mode) != 0 && IsValidFileDirName(szMatch))
	{
	    RetVal = FSOBJ_DIRECTORY;
	    bDone = TRUE;
	}
	else if (IsValidFileDirName(szMatch))
	{
	    RetVal = FSOBJ_FILE;
	    bDone = TRUE;
	}
	else
	{
	    // If we couldn't use this one, find another
	    szMatch = m_pFileFinder->FindNext();
	}

	if (RetVal != FSOBJ_NOTVALID)
	{
	    SafeStrCpy(szPath, szMatchPath, nSize);
	}
    }

    return RetVal;
}

HXBOOL 
CHXDirectory::DeleteFile(const char* szRelPath)
{
    HXBOOL RetVal = FALSE;
    CHXString strPath;

    if(!szRelPath)
        return FALSE;

    strPath = szRelPath;

    chmod((const char*)strPath, S_IREAD | S_IWRITE);
    if(!unlink((const char*)strPath) || errno != EACCES)
        RetVal = TRUE;

    return RetVal;
}

/* Sets itself to current directory. */
HXBOOL 
CHXDirectory::SetCurrentDir()
{
    HXBOOL bRetVal = TRUE;

    if(!getcwd(m_strPath.GetBuffer(_MAX_PATH + 1), _MAX_PATH))
        bRetVal = FALSE;
    m_strPath.ReleaseBuffer();

    return bRetVal;
}

/* Makes itself a current directory. */
HXBOOL 
CHXDirectory::MakeCurrentDir()
{
    if(!chdir((const char*)m_strPath))
        return TRUE;
    return FALSE;
}


UINT32 
CHXDirectory::Rename(const char* szOldName, const char* szNewName)
{
    if ((szOldName == NULL) || (szNewName == NULL))
    {
        HX_ASSERT(FALSE);
        return (UINT32)HXR_FAIL;
    }

    UINT32 theError = (UINT32)HXR_FAIL;
    if(unlink(szNewName) == -1 && errno == EACCES)
    {
        chmod(szNewName, S_IREAD | S_IWRITE);
        if(unlink(szNewName) == -1 && errno == EACCES)
            return (UINT32)HXR_FAIL;                          
        if(!rename(szOldName, szNewName))
            theError = (UINT32)HXR_OK;
        chmod(szNewName, S_IREAD);                  
    }
    else
    {
        if(!rename(szOldName, szNewName))
            theError = (UINT32)HXR_OK;
    }

    return theError;
}

HXBOOL 
CHXDirectory::IsValidFileDirName(const char* szPath)
{
    if(!strcmp(szPath, ".") || !strcmp(szPath, ".."))
        return FALSE;

    return TRUE;
}
