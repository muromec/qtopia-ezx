/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: findfile.h,v 1.10 2005/10/17 23:18:10 rrajesh Exp $
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

//
// findfile.h
//
// Interface to CFindFile.
//
// CFindFile is a class for finding files in a search path.
// The file name to search for can contain wildcards.
//
// regcomp/regcmp is used for the pattern-matching, so the
// appropriate library must be linked in to use this class.
//

#ifndef __FINDFILE_H
#define __FINDFILE_H

#include "hxtypes.h"
#include "hxcom.h"
#include "hxstring.h"

// forward declares for classes and structs
// include for regular expression compiler

#if defined(_UNIX)

#include <sys/types.h>

#if defined (_IRIX)
#include <libgen.h>
#elif  defined (_LINUX)
#include <regex.h>
#endif

#if defined (_SOLARIS) || defined (_FREEBSD) || defined (_OPENBSD) || defined (_OPENBSD) || defined (_NETBSD)
#include <dirent.h>
#elif defined (__hpux)
#include <sys/dirent.h>
#elif defined (_VXWORKS)
#else
#include <sys/dir.h>
#endif

#endif

#if defined (_WINDOWS) || defined (_WIN32) || defined (_SYMBIAN)
#  define OS_PATH_DELIMITER   "\\"
#elif defined (_UNIX) || defined (_OPENWAVE)
#  define OS_PATH_DELIMITER   "/"
#elif defined (_MACINTOSH)
#  define OS_PATH_DELIMITER   ":"
#else
#  define OS_PATH_DELIMITER   "/"
#endif

class CFindFile
{
public:

    static CFindFile*   CreateFindFile( const char *path,
				    const char *delimiter,
				    const char *pattern,
				    IUnknown** ppCommonObj = NULL);

	virtual ~CFindFile();

	    // operations
	    // calls to find first/find next return the filename
	    // without the path information.
	    virtual char *	FindFirst();
	    virtual char *	FindNext ();

	    // queries
	    char*	GetCurFilename ()   { return m_curFilename; } ;
	    char*	GetCurDirectory ()  { return m_curDir; } ;
	    char*	GetCurFilePath()    { return m_pFilePath; };

protected:
	// construct with search path, search path delimiter, and
	// file-matching pattern.
	CFindFile	(const char *path,
				const char *delimiter,
				const char *pattern);

	CHXString	m_searchPath;
	char *	m_searchPathDelimiter;
	char *	m_pattern;
	char *	m_curFilename;
	char *  m_curDir;
	char*	m_pFilePath;

	HXBOOL 	m_started;

	//
	// each OS must define these!
	//

	// the directory methods are used to iterate through
	// the entire contents of a single directory
	virtual HXBOOL	OS_OpenDirectory (const char * dirName) = 0;
	virtual char *	OS_GetNextFile () = 0;
	virtual void	OS_CloseDirectory () = 0;

	// the pattern methods are used to initialize the
	// pattern-matching routines and then compare
	// individual filenames to the specified pattern.
	virtual HXBOOL	OS_InitPattern () = 0;
	virtual HXBOOL	OS_FileMatchesPattern (const char * fname) = 0;
	virtual void 	OS_FreePattern () = 0;
};

#endif // __FINDFILE_H

