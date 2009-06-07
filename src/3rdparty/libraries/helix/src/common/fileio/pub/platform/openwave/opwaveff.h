/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: opwaveff.h,v 1.6 2005/03/14 19:36:31 bobclark Exp $
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
// opwave_findfile.h
//
// Interface to CFindFile.
//
// CFindFile is a class for finding files in a search path.
// The file name to search for can contain wildcards.
//
// regcomp/regcmp is used for the pattern-matching, so the
// appropriate library must be linked in to use this class.
//
//

#ifndef __OPWAVEFF_H
#define __OPWAVEFF_H

#include "hxtypes.h"

// forward declares for classes and structs
// include for regular expression compiler

#include "hlxclib/sys/types.h"
//#include <sys/dir.h>

class COpenwaveFindFile : public CFindFile
{
public:

	COpenwaveFindFile	(const char *path,
						const char *delimiter,
						const char *pattern);

	~COpenwaveFindFile();
protected:
	virtual HXBOOL	OS_OpenDirectory (const char * dirName);
	virtual char *	OS_GetNextFile ();
	virtual void	OS_CloseDirectory ();

	// the pattern methods are used to initialize the
	// pattern-matching routines and then compare
	// individual filenames to the specified pattern.
	virtual HXBOOL	OS_InitPattern ();
	virtual HXBOOL	OS_FileMatchesPattern (const char * fname);
	virtual void 	OS_FreePattern ();

	// throw OS-specific directory and pattern-matching
	// variables here!
//	DIR	 *	m_dirHandle;
//#if (!defined (_SOLARIS) && !defined (_FREEBSD) && !defined (__hpux) && !defined(_AIX) && !defined (_OSF1) && !defined(__QNXNTO__))
//        struct direct *m_dirEntry;
//#else
//        struct dirent *m_dirEntry;
//#endif
};

#endif // __OPWAVEFF_H

