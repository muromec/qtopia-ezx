/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbhxdir.cpp,v 1.11 2007/07/06 20:35:15 jfinnecy Exp $
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

/************************************************************************
 *  Includes
 */
#include "findfile.h"
#include "hxdir.h"
#include "hxheap.h"

#include "hlxosstr.h"
#include "hxstrutl.h"

#include "symbsessionmgr.h"
#include "chxdataf.h"
#include "symbianff.h"


/************************************************************************
 *  Constructor/Destructor
 */
CHXDirectory::CHXDirectory(IUnknown* pContext, IUnknown** ppCommonObj)
: m_pFileFinder(NULL)
, m_pContext(pContext)
, m_pSessionManager(NULL)
{
    HX_ADDREF(m_pContext);
    CSymbSessionMgr::Create(m_pSessionManager, ppCommonObj);
}

CHXDirectory::~CHXDirectory()
{
    HX_DELETE(m_pFileFinder);
    HX_RELEASE(m_pSessionManager);
    HX_RELEASE(m_pContext);
}


HXBOOL CHXDirectory::GetSession(void)
{
    if (m_pSessionManager)
    {
	return (m_pSessionManager->GetSession(m_symbSession) == HXR_OK);
    }
    
    return FALSE;
}

HXBOOL CHXDirectory::GetFileAttributes(const char* szPath, TUint& symbAttValue)
{
    HXBOOL bRetVal = FALSE;

    if (GetSession())
    {
	OS_STRING_TYPE osFileName(szPath);
	TPtrC symbNameDesc((TText*) ((OS_TEXT_PTR) osFileName));
	
	bRetVal = (m_symbSession.Att(symbNameDesc, symbAttValue) == KErrNone);
    }

    return bRetVal;
}

HXBOOL CHXDirectory::IsWritable()
{
    TUint symbAttValue;
    HXBOOL bRetVal = FALSE;
    
    if ((!m_strPath.IsEmpty()) && GetFileAttributes(m_strPath, symbAttValue))
    {
	bRetVal = (!(symbAttValue & KEntryAttReadOnly));
    }

    return bRetVal;
}

HXBOOL CHXDirectory::IsWritable(const char* szPath)
{
    TUint symbAttValue;
    HXBOOL bRetVal = FALSE;
    
    if (GetFileAttributes(szPath, symbAttValue))
    {
	bRetVal = (!(symbAttValue & KEntryAttReadOnly));
    }

    return bRetVal;
}

void CHXDirectory::SetPath(const char* szPath)
{
    if (szPath)
    {	
	m_strPath = szPath;

	// make sure path ends with a '\' 
	if (m_strPath.IsEmpty() || (m_strPath.Right(1) != OS_SEPARATOR_STRING))
	{
	    m_strPath += OS_SEPARATOR_STRING;
	}
    }
}

HXBOOL CHXDirectory::SetTempPath(HXXHANDLE , const char* szRelPath)
{
    // caller must specify a sub-directory
    if ((szRelPath == NULL) || (szRelPath[0] == '\0'))
    {
	return FALSE;
    }

    m_strPath.Empty();

    // try current working directory
    if (!SetCurrentDir() || !IsWritable(m_strPath))
    {
	return FALSE;
    }
    
    // now append the sub-directory, separating if necessary
    if ((m_strPath.IsEmpty() || (m_strPath.Right(1) != OS_SEPARATOR_STRING)) && 
	(szRelPath[0] != OS_SEPARATOR_CHAR))
    {
	m_strPath += OS_SEPARATOR_STRING;
    }
    m_strPath += szRelPath;
    if (m_strPath.Right(1) != OS_SEPARATOR_STRING)
    {
	m_strPath += OS_SEPARATOR_STRING;
    }

    return TRUE;
}

HXBOOL CHXDirectory::Create()
{
    HXBOOL bRetVal = FALSE;

    if ((!m_strPath.IsEmpty()) && GetSession())
    {
	TInt symbError;

	OS_STRING_TYPE osFileName(m_strPath);
	TPtrC symbNameDesc((TText*) ((OS_TEXT_PTR) osFileName));

	symbError = m_symbSession.MkDirAll(symbNameDesc);

	bRetVal = (symbError == KErrNone);
    }

    return bRetVal;
}

HXBOOL CHXDirectory::IsValid()
{
    TUint symbAttValue;
    HXBOOL bRetVal = FALSE;
    
    if ((!m_strPath.IsEmpty()) && GetFileAttributes(m_strPath, symbAttValue))
    {
	bRetVal = ((symbAttValue & KEntryAttDir) != 0);
    }

    return bRetVal;
}

HXBOOL CHXDirectory::DeleteDirectory()
{
    HXBOOL bRetVal = FALSE;

    if ((!m_strPath.IsEmpty()) && GetSession())
    {
	OS_STRING_TYPE osFileName(m_strPath);
	TPtrC symbNameDesc((TText*) ((OS_TEXT_PTR) osFileName));

	bRetVal = (m_symbSession.RmDir(symbNameDesc) == KErrNone);
    }

    return bRetVal;
}

CHXDirectory::FSOBJ CHXDirectory::FindNextEntry(char*  szPath,
						UINT16 nSize,
						const char* szPattern,
						HXBOOL bReset)
{
    FSOBJ RetVal = FSOBJ_NOTVALID;
    char* szMatch = NULL;
    char* szMatchPath = NULL;
    HXBOOL  bDone = FALSE;
    TUint symbAttValue;
    
    // Find the first file that matches the specified pattern
    if (bReset || szPattern || (!m_pFileFinder))
    {
	HX_DELETE(m_pFileFinder); 
	m_pFileFinder = new CSymbianFindFile(m_strPath,
					     0,
					     szPattern,
					     (IUnknown**) (&m_pSessionManager),
					     TRUE); // Find sub-dirs
	if (!m_pFileFinder)
	{
	    return RetVal;	
	}

	szMatch = m_pFileFinder->FindFirst();
    }
    else
    {
	szMatch = m_pFileFinder->FindNext();
    }

    while (szMatch && !bDone)
    {
	szMatchPath = m_pFileFinder->GetCurFilePath();

	if (!GetFileAttributes(szMatchPath, symbAttValue))
	{
	    return RetVal;
	}

	if (symbAttValue & KEntryAttDir)
	{
	    RetVal = FSOBJ_DIRECTORY;
	    bDone = TRUE;
	}
	else if (!(symbAttValue & KEntryAttVolume))
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

CHXDirectory::FSOBJ CHXDirectory::FindFirst(const char* szPattern, char* szPath, UINT16 nSize)
{
    return FindNextEntry(szPath, nSize, szPattern, TRUE);
}

CHXDirectory::FSOBJ CHXDirectory::FindNext(char* szPath, UINT16 nSize)
{
    return FindNextEntry(szPath, nSize);
}

HXBOOL CHXDirectory::DeleteFile(const char* szRelPath)
{
    CHXString strPath;
    HXBOOL bRetVal = FALSE;

    if (!szRelPath)
    {
        return FALSE;
    }

    if (strlen(szRelPath) > 1 && szRelPath[1] == ':')
    {
        strPath = szRelPath;
    }
    else
    {
	strPath = m_strPath;
	strPath += szRelPath;
    }
    CHXDataFile * pSymbCHXFile = CHXDataFile::Construct(m_pContext, 0,(IUnknown**)&m_pSessionManager);
    bRetVal = pSymbCHXFile->Delete(strPath);
    HX_DELETE(pSymbCHXFile);
    return bRetVal;
}

HXBOOL CHXDirectory::SetCurrentDir()
{
    HXBOOL bRetVal = FALSE;

    if (GetSession())
    {
	TFileName* psymbCurrentDir = new TFileName;

	if (psymbCurrentDir)
	{
	    bRetVal = (m_symbSession.SessionPath(*psymbCurrentDir) == KErrNone);

	    if (bRetVal)
	    {
		m_strPath = (const char *) OS_STRING2((OS_TEXT_PTR) psymbCurrentDir->Ptr(), 
						      psymbCurrentDir->Length());
	    }

	    delete psymbCurrentDir;
	}
    }

    return bRetVal;
}

HXBOOL CHXDirectory::MakeCurrentDir()
{
    HXBOOL bRetVal = FALSE;

    HX_ASSERT("MakeCurrentDir Not Working As Expected" == NULL);

    if (GetSession() && (!m_strPath.IsEmpty()))
    {
	OS_STRING_TYPE osFileName(m_strPath);
	TPtrC symbNameDesc((TText*) ((OS_TEXT_PTR) osFileName));
	
	bRetVal = (m_symbSession.SetSessionPath(symbNameDesc) == KErrNone);
	if (bRetVal)
	{
#ifndef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
        // SetDefaultPath is depricated in Symbian 9
	    bRetVal = (m_symbSession.SetDefaultPath(symbNameDesc) == KErrNone);
#endif 
	}
    }

    return bRetVal;
}


UINT32 CHXDirectory::Rename(const char* szOldName, const char* szNewName)
{
    HX_RESULT retVal = HXR_FAIL;

    if (GetSession())
    {
	OS_STRING_TYPE osFileNameOld(szOldName);
	OS_STRING_TYPE osFileNameNew(szNewName);
	TPtrC symbNameDescOld((TText*) ((OS_TEXT_PTR) osFileNameOld));
	TPtrC symbNameDescNew((TText*) ((OS_TEXT_PTR) osFileNameNew));
	
	if (m_symbSession.Rename(symbNameDescOld, symbNameDescNew) == KErrNone)
	{
	    retVal = HXR_OK;
	}
    }

    return retVal;
}

HXBOOL CHXDirectory::IsValidFileDirName(const char* szPath)
{
    HXBOOL bRetVal = FALSE;

    if (GetSession())
    {
	OS_STRING_TYPE osFileName(szPath);
	TPtrC symbNameDesc((TText*) ((OS_TEXT_PTR) osFileName));
	
	bRetVal = m_symbSession.IsValidName(symbNameDesc) ? TRUE : FALSE;
    }

    return bRetVal;
}
