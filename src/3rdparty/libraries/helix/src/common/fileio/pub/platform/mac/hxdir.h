/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdir.h,v 1.6 2005/03/14 19:36:31 bobclark Exp $
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

#ifndef _MACHXDIR_H
#define _MACHXDIR_H

#include"hxbasedir.h"

#ifdef _CARBON
	#include "filespec.h"
#endif

#ifdef _MAC_MACHO
#define OS_SEPARATOR_CHAR	'/'
#define OS_SEPARATOR_STRING	"/"
#else
#define OS_SEPARATOR_CHAR	':'
#define OS_SEPARATOR_STRING	":"
#endif

class CHXDirectory : public XHXDirectory
{
public:
    CHXDirectory();
    ~CHXDirectory();

    virtual void SetPath(const char* szPath);

	/* set the path to something in the temporary items directory */
    virtual HXBOOL SetTempPath(HXXHANDLE hpsHandle, const char* szRelPath);

    /* Creates directory. */
    virtual HXBOOL Create();

    /* Checks if directory exists. */    
    virtual HXBOOL IsValid();

    /* Destroys directory. */
    virtual HXBOOL Destroy(HXBOOL bRemoveContents);

    /* Deletes file. */
    virtual HXBOOL DeleteFile(const char* szRelPath);

    /* Sets itself to current directory. */
    virtual HXBOOL SetCurrentDir();

    /* Makes itself a current directory. */
    virtual HXBOOL MakeCurrentDir();

    /* Checks if directory is on CD or removable drive. */    
    virtual HXBOOL IsRemovable();

    /* Starts enumeration process. */
    virtual FSOBJ FindFirst(const char* szPattern, char* szPath, UINT16 nSize);

    /* Continues enumeration process. */
    virtual FSOBJ FindNext(char* szPath, UINT16 nSize);

    /* renames a file */
    virtual UINT32 Rename(const char* szOldName, const char* szNewName);
    
    /* moves or copies (depending on bMove) file */
    static HXBOOL MoveRename(const char* szSrcName, const char* szDestName, HXBOOL bMove=TRUE);

    /* gets this directory's dirID */
    virtual OSErr GetDirID(long& dirID);

	/* gets a FSSpec for this directory */
    virtual FSSpec GetFSSpec(void)
	{
#ifdef _CARBON
		return (FSSpec) CHXDirSpecifier(m_strPath);
#else
		return m_dirSpec;
#endif
	}

#if defined(_DEBUG) && defined(_CARBON)
	static void TestHXDir();
#endif

protected:

#ifdef _CARBON
	FSIterator m_FSIterator;
	CHXString m_strFindPattern;
#else
	// To keep the directory object valid even if the user renames
	// the drive or the parent folders, we rely on the FSSpec
	// rather than the path name internally.  This should always
	// be updated when 
	//
	// However, it's possible for a path to be set which can't
	// be represented in a FSSpec because the parent folders
	// don't yet exist.
	
   	FSSpec	m_dirSpec;	// same directory specified in m_strPath
   	
	FSSpec	m_fileSpec;
	
    ULONG32	m_nIndex;
    char*	m_pNextFileName;
    char*	m_pCurrentFileName;
    char* 	m_pPattern;
    INT16	m_nDefaultVRefNum;
    UINT16	m_nDirItems;
    virtual HXBOOL EnsureValidPathSpec();
#endif

    /* Deletes empty directory. */
    virtual HXBOOL DeleteDirectory();
    
    /* If the m_dirSpec isn't properly set because SetPath sets it
       to something so speculative that there's no parent folder,
       we can use this function to later ensure that it's valid. */
       
};

#endif // _MACHXDIR_H

