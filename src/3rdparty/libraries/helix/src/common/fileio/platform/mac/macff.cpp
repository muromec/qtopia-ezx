/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: macff.cpp,v 1.6 2005/03/14 19:36:29 bobclark Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#include <string.h>
#include "platform/mac/macff.h"
#include "pn_morefiles.h"
#include "hxstrutl.h"
#include "hxstring.h"
#include "fullpathname.h"

#ifndef _CARBON
#include "morefilesextras.h" // for FSpGetDirectoryID, which is in pn_morefiles under Carbon
#endif

#ifndef _MAX_PATH
#define _MAX_PATH 255
#endif

CMacFindFile::CMacFindFile (	const char *path,
				const char *delimiter,
				const char *pattern) :
    CFindFile (path, delimiter, pattern)
{
    m_pCurrentDirectory	    = NULL;
    m_pCurrentFileName	    = new char[_MAX_PATH];
    *m_pCurrentFileName = 0;
    m_pNextFileName	    = new char[_MAX_PATH];
    *m_pNextFileName = 0;
    m_nIndex = 0;
    m_VRefNum = 0;
    m_DirID = 0;
}

CMacFindFile::~CMacFindFile()
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
}

//
// Open the directory; initialize the directory handle.
// Return FALSE if the directory couldn't be opened.
//
HXBOOL CMacFindFile::OS_OpenDirectory (const char *dirname)
{
    HXBOOL bReturn = FALSE;
    
    if (m_pCurrentDirectory)
    {
	delete [] m_pCurrentDirectory;
	m_pCurrentDirectory = 0;
    }

    UINT16 ulPatLen = 0;

    /* 1 for \ and 1 for NULL at end */
    m_pCurrentDirectory = new char[strlen(dirname) + ulPatLen + 2];
    if (!m_pCurrentDirectory)
	return FALSE;

    strcpy(m_pCurrentDirectory, dirname); /* Flawfinder: ignore */

    FSSpec dirSpec;
    OSErr err = FSSpecFromPathName (dirname, &dirSpec); 
    if (err == noErr)
    {
	Boolean isDir;
	
	m_VRefNum = dirSpec.vRefNum;
	
	err = FSpGetDirectoryID(&dirSpec, &m_DirID, &isDir);
    }
    
    if (err == noErr)
    {
	m_nIndex = 0;
	bReturn = TRUE;
	HXBOOL bContinue;
	FSSpec nextFSpec;
	while (noErr == FSpGetNthDirectoryItem(m_VRefNum, m_DirID, ++m_nIndex, &nextFSpec))
	{
	    bContinue = TRUE;
	    if (!m_pNextFileName)
	    {
		m_pNextFileName	    = new char[_MAX_PATH];
		if (!m_pNextFileName)
		{
		    return FALSE;
		}
		*m_pNextFileName = 0;
	    }
	    
	    if (!m_pCurrentFileName)
	    {
		m_pCurrentFileName	    = new char[_MAX_PATH];
		if (!m_pCurrentFileName)
		{
		    return FALSE;
		}
		*m_pCurrentFileName = 0;
	    }
	    // matches pattern?
	    int len = nextFSpec.name[0];
	    INT16 patternLen = strlen(m_pattern)-1;
	    if (0 == strcmp("*.*", m_pattern))
	    	bContinue = TRUE;
	    else
	    {
	    if (m_pattern[0] == '*')
	    {
		    if (0 != strncasecmp((char*)&nextFSpec.name[len-patternLen+1], &m_pattern[1], patternLen))
		    	bContinue = FALSE;
	    }
	    else if (m_pattern[patternLen-1] == '*')
	    {
	    	    if (0 != strncasecmp((char*)&nextFSpec.name[1], &m_pattern[0], patternLen))
	    	    	bContinue = FALSE;
	    }
	    else if (strncasecmp((char*)&nextFSpec.name[1], &m_pattern[0], patternLen))
	    	bContinue = FALSE;
	    }
	    
	    if (bContinue)
	    {
	    	CHXString tempStr;
	    	tempStr = nextFSpec; // copy full path
	    	SafeStrCpy(m_pNextFileName, (const char*)tempStr, _MAX_PATH);
	    	return TRUE;
	    }
	}
    }
    // if we haven't returned yet, return TRUE if dir is valid & no pattern supplied
    if (bReturn)
    {
    	if (m_pattern)
    	    bReturn = FALSE;
    }
    return bReturn;
}

//
// Get the next file in the directory. Filters according to pattern
//
char* CMacFindFile::OS_GetNextFile()
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
	FSSpec nextFSpec;
	HXBOOL bContinue;
	HXBOOL bFound = FALSE;
	INT16 len = 0;
	while (noErr == FSpGetNthDirectoryItem(m_VRefNum, m_DirID, ++m_nIndex, &nextFSpec))
	{
	    bContinue = TRUE;
	    // matches pattern(extension)? (eg. DLL)
	    len = nextFSpec.name[0]; // pascal str
	    if (m_pattern)
	    {
		INT16 patternLen = strlen(m_pattern)-1;
	    	if (0 == strcmp("*.*", m_pattern))
	    	    bContinue = TRUE;
	    	else
	    	{
		if (m_pattern[0] == '*')
		{
		    if (0 != strncasecmp((char*)&nextFSpec.name[len-patternLen+1], &m_pattern[1], patternLen))
		    	bContinue = FALSE;
		}
		else if (m_pattern[patternLen-1] == '*')
		{
	    	    if (0 != strncasecmp((char*)&nextFSpec.name[1], &m_pattern[0], patternLen))
	    	    	bContinue = FALSE;
	    	}
	    	}
	    }
	    if (bContinue)
	    {
	    	CHXString tempStr;
	    	tempStr = nextFSpec; // copy full path
	        SafeStrCpy(m_pNextFileName, (const char*)tempStr, _MAX_PATH);
	    	bFound = TRUE;
	    	break;
	    }
	}
	if (!bFound)
	{
	    delete [] m_pNextFileName;
	    m_pNextFileName = 0;
	}
    }
#ifdef _CARBON
	//xxxbobclark I think this is crashing because on the final iteration
	// it's an empty string and is thus returning nil somehow.

	static char* sEmptyString = "\0";
	
	char* curFName = m_pCurrentFileName;
	if (!curFName)
	{
		curFName = sEmptyString;
	}
    char* fName = strrchr(curFName,':');
#else
    char* fName = strrchr(m_pCurrentFileName,':');
#endif
    if (fName) 
    	++fName;
    else
    	fName = m_pCurrentFileName;
    return fName;
}

//
// release the directory
//
void CMacFindFile::OS_CloseDirectory ()
{
    return;
}

HXBOOL CMacFindFile::OS_InitPattern ()
{
    return TRUE;
}

HXBOOL CMacFindFile::OS_FileMatchesPattern (const char * fname)
{
    return TRUE;
}

void CMacFindFile::OS_FreePattern ()
{
    return;
}

// ------------------------------------------------------------------------------------
