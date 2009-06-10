/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdir.cpp,v 1.10 2007/07/06 20:35:17 jfinnecy Exp $
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hlxclib/errno.h"
#include "hlxclib/io.h"
#include "hlxclib/sys/stat.h"
#include "hxstrutl.h"
#include "hxdir.h"
#include "dbcs.h"

#if _WIN16
#include <direct.h>
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

CHXDirectory::CHXDirectory()
#ifdef _WIN32
            : m_hFindFile(INVALID_HANDLE_VALUE)
#endif
{
}

CHXDirectory::~CHXDirectory()
{
#ifdef _WIN32
    if(m_hFindFile != INVALID_HANDLE_VALUE)
    {
        FindClose(m_hFindFile);
        m_hFindFile = INVALID_HANDLE_VALUE;
    }
#endif
}

void 
CHXDirectory::SetPath(const char* szPath)
{
    XHXDirectory::SetPath(szPath);
    if(m_strPath.GetLength() == 2 && m_strPath.GetAt(1) == ':')
	m_strPath += "\\";
    // Remove ending '\'.
    const char* pLastChar = HXGetPrevChar(szPath, szPath + strlen(szPath));
    if(m_strPath.GetLength() > 3 && pLastChar + 1 == szPath + strlen(szPath) && *pLastChar == '\\')
	m_strPath = m_strPath.Left(m_strPath.GetLength() - 1);
}

static HXBOOL HasDiskSpaceFree(const char* pPath)
{
    HXBOOL bRet = FALSE;

#if defined(WIN32_PLATFORM_PSPC)
    ULARGE_INTEGER   lpFreeBytesAvailableToCaller;
    ULARGE_INTEGER   lpTotalNumberOfBytes;
    ULARGE_INTEGER   lpTotalNumberOfFreeBytes;

    GetDiskFreeSpaceEx(OS_STRING(pPath), &lpFreeBytesAvailableToCaller,
		       &lpTotalNumberOfBytes,
		       &lpTotalNumberOfFreeBytes);

    if (lpFreeBytesAvailableToCaller.LowPart ||
	lpFreeBytesAvailableToCaller.HighPart)
	bRet = TRUE;

#else /* defined(WIN32_PLATFORM_PSPC) */
    DWORD nSectorsPerCluster = 0;
    DWORD nBytesPerSector = 0; 
    DWORD nNumberOfFreeClusters = 0;
    DWORD TotalNumberOfClusters = 0;

    GetDiskFreeSpace(OS_STRING(pPath), 
		     &nSectorsPerCluster, 
		     &nBytesPerSector, 
		     &nNumberOfFreeClusters, 
		     &TotalNumberOfClusters);
    
    if (nNumberOfFreeClusters)
	bRet = TRUE;
#endif /* defined(WIN32_PLATFORM_PSPC) */
    return bRet;
}

HXBOOL
CHXDirectory::IsWritable()
{
    if(!IsValid())
	return FALSE;

    int nIndex = m_strPath.Find(":\\");
    if(nIndex != -1)
    {
	CHXString strDiskRootDir = m_strPath.Left(nIndex + 2);

	if(HasDiskSpaceFree(strDiskRootDir))
	    return TRUE;
    }

    return FALSE;
}
 
HXBOOL
CHXDirectory::SetTempPath(HXXHANDLE hpsHandle, const char* szRelPath)
{
    HXBOOL bTempDirSet = FALSE;

    /* First try to use TEMP environment variable as extraction path. */
    char * pTEMPValue = 0;

#if !defined(WIN32_PLATFORM_PSPC)
    pTEMPValue = getenv("TEMP");
#endif /* !defined(WIN32_PLATFORM_PSPC) */

    if(pTEMPValue)
    {
        m_strPath = pTEMPValue;

	if(IsWritable())
	{
	    bTempDirSet = TRUE;
	}
	else
	{
	    m_strPath.Empty();
	}
    }

    /* If we don't have suitable path 
       we will try to use application directory. */
    if(!bTempDirSet && hpsHandle)
    {
        GetModuleFileName((HINSTANCE)hpsHandle, 
			  OS_STRING2(m_strPath.GetBuffer(_MAX_PATH + 1), 
				     _MAX_PATH),
			   _MAX_PATH);
        m_strPath.ReleaseBuffer();
        int nIndex = m_strPath.ReverseFind('\\');
        if(nIndex == -1)
        {
            m_strPath.Empty();
        }
        else
        {
            m_strPath = m_strPath.Left(nIndex);
        }

	if(IsWritable())
	{
	    bTempDirSet = TRUE;
	}
	else
	{
	    m_strPath.Empty();
	}
    }

    /* If we don't have suitable path 
       we will try to use Windows directory. */
    if(!bTempDirSet)
    {
#if defined(WIN32_PLATFORM_PSPC)
	// WinCE does not have the GetWindowsDirectory() call,
	// so the path is hard coded here
	m_strPath = "\\Windows";
#else /* defined(WIN32_PLATFORM_PSPC) */
        GetWindowsDirectory(m_strPath.GetBuffer(_MAX_PATH + 1), _MAX_PATH);
	m_strPath.ReleaseBuffer();
#endif /* defined(WIN32_PLATFORM_PSPC) */

	if(IsWritable())
	{
	    bTempDirSet = TRUE;
	}
	else
	{
	    m_strPath.Empty();
	}
    }

    if(bTempDirSet && szRelPath && szRelPath[0])
    {
	const char* pLastChar = HXGetPrevChar(m_strPath, (const char*)m_strPath + m_strPath.GetLength());
	if(pLastChar + 1 != (const char*)m_strPath + m_strPath.GetLength() || *pLastChar != '\\')
	    m_strPath += "\\";
        m_strPath += szRelPath;
    }

    return bTempDirSet;
}

/* Creates directory. */
HXBOOL 
CHXDirectory::Create()
{
    CreateDirectory(OS_STRING(m_strPath), 0);
    return IsValid();
}

/* Checks if directory exists. */    
HXBOOL 
CHXDirectory::IsValid()
{
    if(m_strPath.IsEmpty() || m_strPath.GetAt(0) == '.')
        return FALSE;

    if(m_strPath.GetLength() == 3 && m_strPath.GetAt(1) == ':' &&
       m_strPath.GetAt(2) == '\\')
    {
#if defined(WIN32_PLATFORM_PSPC)
	return TRUE;
#else /* defined(WIN32_PLATFORM_PSPC) */
	UINT nDriveType = 0;

#ifdef _WIN32
	nDriveType = GetDriveType(m_strPath);
#else
        nDriveType = GetDriveType(toupper(m_strPath.GetAt(0)) - 'A');
#endif
	if(nDriveType == DRIVE_FIXED || nDriveType == DRIVE_REMOTE ||
	   nDriveType == DRIVE_REMOVABLE || nDriveType == DRIVE_RAMDISK)
	   return TRUE;
#endif /* defined(WIN32_PLATFORM_PSPC) */
    }
    else
    {
#ifdef _WIN32
	WIN32_FIND_DATA findData;

	//  If a network share is requested, add a '*' to the end of the path.
	//  This is necessary since FindFirstFile must have a star at the end for
	//  network shares...per API documentation:
	//	" Similarly, on network shares, you can use an lpFileName of the form 
	//	"\\server\service\*" but you cannot use an lpFileName that points to the 
	//	share itself, such as "\\server\service" ".

	CHXString strTempPath = m_strPath;
	if(strTempPath.Find("\\") == 0)
	{
	    if(strTempPath.ReverseFind('\\') != (strTempPath.GetLength() - 1))
	    {
		strTempPath += "\\";
	    }

	    strTempPath += "*";
	}

	HANDLE hFindFile =  FindFirstFile(OS_STRING(strTempPath), &findData);
	if(hFindFile != INVALID_HANDLE_VALUE)
	{
	    FindClose(hFindFile);
	    if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		return TRUE;
	}
#else
	struct _find_t fileinfo;
	if(!_dos_findfirst(m_strPath, _A_SUBDIR | _A_HIDDEN, &fileinfo ))
	{   
	    if (fileinfo.attrib & _A_SUBDIR)
		return TRUE;
	}
#endif
    }

    return FALSE;
}

/* Checks if directory is on CD or removable drive. */    
HXBOOL 
CHXDirectory::IsRemovable()
{
    if(m_strPath.GetLength() < 2 || m_strPath.GetAt(1) != ':')
        return FALSE;

#if !defined(WIN32_PLATFORM_PSPC)
    UINT nDriveType = 0;
#ifdef _WIN32
    nDriveType = GetDriveType(m_strPath);
    if(nDriveType == DRIVE_CDROM)
       return TRUE;
#else
    nDriveType = GetDriveType(toupper(m_strPath.GetAt(0)) - 'A');
#endif
    if(nDriveType == DRIVE_REMOVABLE)
       return TRUE;
#endif /* defined(WIN32_PLATFORM_PSPC) */

    return FALSE;
}

/* Deletes empty directory. */
HXBOOL 
CHXDirectory::DeleteDirectory()
{
#ifdef _WIN32
    if(m_hFindFile != INVALID_HANDLE_VALUE)
    {
        FindClose(m_hFindFile);
        m_hFindFile = INVALID_HANDLE_VALUE;
    }
#endif

    if(RemoveDirectory(OS_STRING(m_strPath)))
        return TRUE;
    return FALSE;
}

/* Starts enumeration process. */
CHXDirectory::FSOBJ 
CHXDirectory::FindFirst(const char* szPattern, char* szPath, UINT16 nSize)
{
    FSOBJ RetVal = FSOBJ_NOTVALID;

    CHXString strMask = m_strPath;
    const char* pLastChar = HXGetPrevChar(m_strPath, (const char*)m_strPath + m_strPath.GetLength());
    if(pLastChar + 1 != (const char*)m_strPath + m_strPath.GetLength() || *pLastChar != '\\')
	strMask += "\\";
    strMask += szPattern;

#ifdef _WIN32
    if(m_hFindFile != INVALID_HANDLE_VALUE)
    {
        FindClose(m_hFindFile);
        m_hFindFile = INVALID_HANDLE_VALUE;
    }

    WIN32_FIND_DATA findData;
    m_hFindFile =  FindFirstFile(OS_STRING(strMask), &findData);
    if(m_hFindFile != INVALID_HANDLE_VALUE)
    {
        if(m_strPath.GetLength() + 1 + strlen(OS_STRING(findData.cFileName)) < nSize)
	{
	    SafeStrCpy(szPath, (const char*)m_strPath, nSize);
	    pLastChar = HXGetPrevChar(szPath, szPath + strlen(szPath));
	    if(pLastChar + 1 < szPath + strlen(szPath) || *pLastChar != '\\')
		SafeStrCat(szPath,  "\\", nSize);
            SafeStrCat(szPath,  OS_STRING(findData.cFileName), nSize);
	}

	if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	    RetVal = FSOBJ_DIRECTORY;
        else
            RetVal = FSOBJ_FILE;
    }
#else
    if(!_dos_findfirst(strMask, _A_SUBDIR | _A_HIDDEN, &m_FindFileInfo))
    {   
        if(m_strPath.GetLength() + 1 + strlen(m_FindFileInfo.name) < nSize)
	{
	    SafeStrCpy(szPath, (const char*)m_strPath, nSize);
	    if(m_strPath.GetLength() && m_strPath.GetAt(m_strPath.GetLength() - 1) != '\\')
		SafeStrCat(szPath,  "\\", nSize);
            SafeStrCat(szPath,  m_FindFileInfo.name, nSize);
	}

	if (m_FindFileInfo.attrib & _A_SUBDIR)
            RetVal = FSOBJ_DIRECTORY;
        else
            RetVal = FSOBJ_FILE;
    }
#endif

    while(RetVal != FSOBJ_NOTVALID &&
          !IsValidFileDirName(szPath))
        RetVal = FindNext(szPath, nSize);

    return RetVal;
}

/* Continues enumeration process. */
CHXDirectory::FSOBJ 
CHXDirectory::FindNext(char* szPath, UINT16 nSize)
{
    FSOBJ RetVal = FSOBJ_NOTVALID;

#ifdef _WIN32
    if(m_hFindFile != INVALID_HANDLE_VALUE)
    {
        WIN32_FIND_DATA findData;
        if(FindNextFile(m_hFindFile, &findData))
        {
            if(m_strPath.GetLength() + 1 + strlen(OS_STRING(findData.cFileName)) < nSize)
	    {
		SafeStrCpy(szPath, (const char*)m_strPath, nSize);
		const char* pLastChar = HXGetPrevChar(szPath, szPath + strlen(szPath));
		if(pLastChar + 1 < szPath + strlen(szPath) || *pLastChar != '\\')
		    SafeStrCat(szPath,  "\\", nSize);
		SafeStrCat(szPath,  OS_STRING(findData.cFileName), nSize);
	    }

	    if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	        RetVal = FSOBJ_DIRECTORY;
            else
                RetVal = FSOBJ_FILE;
        }
    }
#else
    if(!_dos_findnext(&m_FindFileInfo))
    {   
        if(m_strPath.GetLength() + 1 + strlen(m_FindFileInfo.name) < nSize)
	{
	    SafeStrCpy(szPath, (const char*)m_strPath, nSize);
	    if(m_strPath.GetLength() && m_strPath.GetAt(m_strPath.GetLength() - 1) != '\\')
		SafeStrCat(szPath,  "\\", nSize);
            SafeStrCat(szPath,  m_FindFileInfo.name, nSize);
	}

	if (m_FindFileInfo.attrib & _A_SUBDIR)
            RetVal = FSOBJ_DIRECTORY;
        else
            RetVal = FSOBJ_FILE;
    }
#endif

    while(RetVal != FSOBJ_NOTVALID &&
          !IsValidFileDirName(szPath))
        RetVal = FindNext(szPath, nSize);

    return RetVal;
}

HXBOOL 
CHXDirectory::IsValidFileDirName(const char* szPath)
{
    const char* pSlash = HXReverseFindChar(szPath, '\\');
    if(!pSlash)
        return FALSE;

    CHXString strEnding = pSlash + 1;
    if(!strEnding.Compare(".") || !strEnding.Compare(".."))
        return FALSE;

    return TRUE;
}

HXBOOL 
CHXDirectory::DeleteFile(const char* szRelPath)
{
    if(!szRelPath)
        return FALSE;

    CHXString strPath;
    if(strlen(szRelPath) > 1 && szRelPath[1] == ':')
    {
        strPath = szRelPath;
    }
    else
    {
	strPath = m_strPath;
	const char* pLastChar = HXGetPrevChar(m_strPath, (const char*)m_strPath + m_strPath.GetLength());
	if(pLastChar + 1 < (const char*)m_strPath + m_strPath.GetLength() || *pLastChar != '\\')
	    strPath += "\\";
	strPath += szRelPath;
    }

    HXBOOL RetVal = FALSE;

    //_chmod(strPath, S_IREAD | S_IWRITE);
    DWORD attrib = GetFileAttributes(OS_STRING(strPath)); 
    attrib &= ~FILE_ATTRIBUTE_READONLY;
    SetFileAttributes(OS_STRING(strPath), attrib); 

    //if(!unlink(strPath) || errno != EACCES)
    if (::DeleteFile(OS_STRING(strPath)))
        RetVal = TRUE;

    return RetVal;
}

/* Sets itself to current directory. */
HXBOOL 
CHXDirectory::SetCurrentDir()
{
    HXBOOL bRetVal = TRUE;

#if defined(WIN32_PLATFORM_PSPC)
    // WinCE doesn't have a concept of a working directory.
    // Assert here and look at the calling code to see
    // if we really need to worry about this
    HX_ASSERT(!"CHXDirectory::SetCurrentDir()");
#else /* defined(WIN32_PLATFORM_PSPC) */
    if(!getcwd(m_strPath.GetBuffer(_MAX_PATH + 1), _MAX_PATH))
	bRetVal = FALSE;
    m_strPath.ReleaseBuffer();
#endif /* defined(WIN32_PLATFORM_PSPC) */

    return bRetVal;
}

/* Makes itself a current directory. */
HXBOOL 
CHXDirectory::MakeCurrentDir()
{

#ifdef _WIN16
    // In win16, you have to change the current drive seperately from changing
    // to the new directory, if the new directory is on a different drive than
    // the current drive.
    int retval = 0;
    int curdrive = _getdrive();
    int drive = (toupper(m_strPath.GetAt(0)) - 'A') + 1;
    if (curdrive != drive)
	if ((retval = _chdrive(drive)) != 0)
	    return FALSE;
#endif

#if defined(WIN32_PLATFORM_PSPC)
    // WinCE does not have the concept of a current directory.
    // Assert here and look at the calling code to see if
    // we need to worry about it
    HX_ASSERT(!"CHXDirectory::MakeCurrentDir()");
#else /* defined(WIN32_PLATFORM_PSPC) */
    if(!chdir(m_strPath))
        return TRUE;
#endif /* defined(WIN32_PLATFORM_PSPC) */

    return FALSE;
}


UINT32 
CHXDirectory::Rename(const char* szOldName, const char* szNewName)
{
    if ((szOldName == NULL) || (szNewName == NULL))
    {
        HX_ASSERT(FALSE);
	return HXR_FAIL;
    }

    UINT32 theError = HXR_FAIL;


    CHXDirectory DestinationDir;
    DestinationDir.SetPath( szNewName );

    if ( DestinationDir.IsValid() )
    {
        //Destination directory exist.
	if(!::RemoveDirectory(OS_STRING(szNewName)) )
	{
	    //Failed to delete destination directory.
	    //Check if read only attribute is set.
	    DWORD attrib = GetFileAttributes(OS_STRING(szNewName));
	    if ( attrib & FILE_ATTRIBUTE_READONLY )
	    {
		//Directory is read only.
		//Remove read only attribute.
		attrib &= ~FILE_ATTRIBUTE_READONLY;
		SetFileAttributes(OS_STRING(szNewName), attrib); 
		
		if(!::RemoveDirectory(OS_STRING(szNewName)))
		{
		    //Still failed to delete.
		    //So put the read only attribute back.
    		    attrib |= FILE_ATTRIBUTE_READONLY;
		    SetFileAttributes(OS_STRING(szNewName), attrib); 

		    return HXR_FAIL;
		}
	    }
	    else
	    {	
		//Failed to delete & directory is not readonly.
		return HXR_FAIL;
	    }
	}
    }

    if(MoveFile(OS_STRING(szOldName), OS_STRING(szNewName)))
	theError = HXR_OK;


    return theError;
}

