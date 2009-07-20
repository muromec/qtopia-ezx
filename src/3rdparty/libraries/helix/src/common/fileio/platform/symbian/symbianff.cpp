/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbianff.cpp,v 1.9 2007/07/06 20:35:15 jfinnecy Exp $
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


/************************************************************************
 *  Includes
 */
#include <stdlib.h>
#include "hlxclib/string.h"

#include "hxassert.h"

#include "findfile.h"
#include "hxdir.h"
#include "platform/symbian/symbianff.h"
#include "symbsessionmgr.h"


static int pmatch(const char* pattern, const char* string);


/************************************************************************
 *  CSymbianFindFile
 */
CSymbianFindFile::CSymbianFindFile (  const char *path,
                                      const char *delimiter,
                                      const char *pattern,
				      IUnknown** ppCommonObj,
				      HXBOOL bFindDirs)
    : CFindFile(path, delimiter, pattern)
    , m_pSessionManager(NULL)
    , m_bOpen(FALSE)
    , m_bFindDirs(bFindDirs)
{
    CSymbSessionMgr::Create(m_pSessionManager, ppCommonObj);

    // make sure path ends with a '\' 
    if (m_searchPath.IsEmpty() || (m_searchPath.Right(1) != OS_SEPARATOR_STRING))
    {
	m_searchPath += OS_SEPARATOR_STRING;
    }
}


CSymbianFindFile::~CSymbianFindFile()
{
    Close();
    HX_RELEASE(m_pSessionManager);
}

HXBOOL CSymbianFindFile::GetSession(void)
{
    if (m_pSessionManager)
    {
	return (m_pSessionManager->GetSession(m_symbSession) == HXR_OK);
    }
    
    return FALSE;
}

HXBOOL CSymbianFindFile::OS_OpenDirectory(const char *dirname)
{
    HXBOOL bRetVal = FALSE;

    if (GetSession())
    {
	OS_STRING_TYPE osFileName(dirname);
	TPtrC symbNameDesc((TText*) ((OS_TEXT_PTR) osFileName));

	OS_CloseDirectory();

	bRetVal = (m_symbDir.Open(m_symbSession, 
				  symbNameDesc, 
				  m_bFindDirs ? KEntryAttDir : KEntryAttNormal) == KErrNone);

	m_bOpen = bRetVal;
    }

    return bRetVal;
}

char* CSymbianFindFile::OS_GetNextFile()
{
    char* pFileName = NULL;

    if (m_bOpen && GetSession())
    {
	TEntry* psymbDirEntry = new TEntry;

	if (psymbDirEntry)
	{
	    HXBOOL bTryAgain;

	    do
	    {
		bTryAgain = FALSE;

		if (m_symbDir.Read(*psymbDirEntry) == KErrNone)
		{
		    m_DirFileName = OS_STRING2((OS_TEXT_PTR) psymbDirEntry->iName.Ptr(),
					       psymbDirEntry->iName.Length());
		    pFileName = m_DirFileName;
		}
	    } while (bTryAgain);

	    delete psymbDirEntry;
	}
    }

    return pFileName;
}

void CSymbianFindFile::OS_CloseDirectory()
{
    Close();
}

void CSymbianFindFile::Close()
{
    if (m_bOpen)
    {
	m_symbDir.Close();

	m_bOpen = FALSE;
    }
}

HXBOOL CSymbianFindFile::OS_InitPattern()
{
    return TRUE;
}

HXBOOL CSymbianFindFile::OS_FileMatchesPattern (const char * fname)
{
    return pmatch(m_pattern, fname);
}

void CSymbianFindFile::OS_FreePattern ()
{
    ;
}


/************************************************************************
 *  Tools
 */
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

