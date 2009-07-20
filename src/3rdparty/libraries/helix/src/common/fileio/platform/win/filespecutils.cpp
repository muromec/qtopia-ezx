/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: filespecutils.cpp,v 1.12 2006/05/19 17:19:55 ping Exp $
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

#include "hxstring.h"
#include "filespecutils.h"
#include "hxtick.h"
#include "hxrand.h"
#include "dbcs.h"

#include "hlxclib/windows.h"
#include <winbase.h>
#include <shlobj.h>
#include "hlxclib/sys/stat.h"

#include "hlxclib/stdio.h"
#include "hlxosstr.h"
#include "hlxclib/errno.h"

//////////////////////////////////////////////////////////
//
// Utility base class -- CHXFileSpecUtils
//
//////////////////////////////////////////////////////////

typedef HXBOOL (HXEXPORT_PTR FPGetFreeSpace) (LPCTSTR lpDir,PULARGE_INTEGER lpFree,PULARGE_INTEGER lpTotBytes,PULARGE_INTEGER lpTotFree);

typedef HRESULT (HXEXPORT_PTR FPSHGetSpecialFolderLocation) (HWND hwndOwner, int nFolder, LPITEMIDLIST *ppidl);  
typedef HRESULT (HXEXPORT_PTR FPSHGetPathFromIDList)(LPCITEMIDLIST pidl, LPSTR pszPath);
typedef	HRESULT (HXEXPORT_PTR FPSHGetMalloc)(LPMALLOC *ppMalloc);

//******************************************************************************
HX_RESULT CHXFileSpecUtils::GetFreeSpaceOnDisk(const CHXDirSpecifier& volSpec, 
					       INT64& freeSpace)
{
	CHXDirSpecifier dirRoot = volSpec.GetVolume();
	CHXString strDir = dirRoot.GetPathName();
	OSVERSIONINFO ver;
	HX_RESULT res = HXR_FAIL;
	WORD w;

	// figure out which version of windows the user is running 
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&ver);
	w = (WORD)ver.dwBuildNumber;
	if (ver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && w <= 1000)
	{
#if !defined(WIN32_PLATFORM_PSPC)
		// the original windows 95
		DWORD dwSecPerClu,dwBytPerSec,dwNumFreeClu,dwNumOfClu;
		if (GetDiskFreeSpace(strDir,&dwSecPerClu,&dwBytPerSec,&dwNumFreeClu,&dwNumOfClu) != 0)
		{
			return HXR_FAIL;
		}

		// do a bit a math to figure out the free space - 
		freeSpace = ((INT64)dwSecPerClu) * ((INT64)dwBytPerSec) * ((INT64)dwNumFreeClu);

#else /* !defined(WIN32_PLATFORM_PSPC) */
		ULARGE_INTEGER   freeBytesAvailableToCaller;
		ULARGE_INTEGER   totalNumberOfBytes;
		ULARGE_INTEGER   totalNumberOfFreeBytes;
		if (!GetDiskFreeSpaceEx(OS_STRING(strDir),
					&freeBytesAvailableToCaller,
					&totalNumberOfBytes,
					&totalNumberOfFreeBytes))
		    return HXR_FAIL;
		freeSpace = freeBytesAvailableToCaller.QuadPart;

#endif /* !defined(WIN32_PLATFORM_PSPC) */
	}
	else
	{
		// windows 95 osr2 or above or winnt
		// the docs for GetDiskFreeSpaceEx say to load kernel32.dll,
		// search for the procaddress and then call the function...

		HINSTANCE hKernel = ::LoadLibrary(OS_STRING("kernel32.dll"));
		if (hKernel)
		{
			FPGetFreeSpace fpGetFreeSpace = (FPGetFreeSpace) GetProcAddress(hKernel, OS_STRING("GetDiskFreeSpaceExA"));
			if (fpGetFreeSpace)
			{
				ULARGE_INTEGER uFreeBytesCaller,uTotNumBytes,uTotNumFreeBytes;

				HXBOOL b = fpGetFreeSpace(OS_STRING(strDir),&uFreeBytesCaller,&uTotNumBytes,&uTotNumFreeBytes);
				if (b)
				{
					// free space on disk or for the user? That is the question
					freeSpace = uFreeBytesCaller.QuadPart;
					res = HXR_OK;
				}
			}

			::FreeLibrary(hKernel);
		}
	}

	return res;
}

//******************************************************************************
HX_RESULT CHXFileSpecUtils::GetTotalSpaceOnDisk(const CHXDirSpecifier& volSpec, 
						INT64& totalSpace)
{ 
	CHXDirSpecifier dirRoot = volSpec.GetVolume();
	CHXString strDir = dirRoot.GetPathName();
	OSVERSIONINFO ver;
	HX_RESULT res = HXR_FAIL;
	WORD w;

	// figure out which version of windows the user is running 
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&ver);
	w = (WORD)ver.dwBuildNumber;
	if (ver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && w <= 1000)
	{
#if !defined(WIN32_PLATFORM_PSPC)
		// the original windows 95
		DWORD dwSecPerClu,dwBytPerSec,dwNumFreeClu,dwNumOfClu;
		if (GetDiskFreeSpace(strDir,&dwSecPerClu,&dwBytPerSec,&dwNumFreeClu,&dwNumOfClu) != 0)
		{
			return HXR_FAIL;
		}

		// do a bit a math to figure out the free space - 
		totalSpace = ((INT64)dwSecPerClu) * ((INT64)dwBytPerSec) * ((INT64)dwNumOfClu);
#else /* !defined(WIN32_PLATFORM_PSPC) */
		ULARGE_INTEGER   freeBytesAvailableToCaller;
		ULARGE_INTEGER   totalNumberOfBytes;
		ULARGE_INTEGER   totalNumberOfFreeBytes;
		if (!GetDiskFreeSpaceEx(OS_STRING(strDir),
					&freeBytesAvailableToCaller,
					&totalNumberOfBytes,
					&totalNumberOfFreeBytes))
		    return HXR_FAIL;
		totalSpace = totalNumberOfBytes.QuadPart;
#endif /* !defined(WIN32_PLATFORM_PSPC) */
	}
	else
	{
		// windows 95 osr2 or above or winnt
		// the docs for GetDiskFreeSpaceEx say to load kernel32.dll,
		// search for the procaddress and then call the function...

		HINSTANCE hKernel = ::LoadLibrary(OS_STRING("kernel32.dll"));
		if (hKernel)
		{
			FPGetFreeSpace fpGetFreeSpace = (FPGetFreeSpace)GetProcAddress(hKernel, OS_STRING("GetDiskFreeSpaceExA"));
			if (fpGetFreeSpace)
			{
				ULARGE_INTEGER uFreeBytesCaller,uTotNumBytes,uTotNumFreeBytes;

				HXBOOL b = fpGetFreeSpace(OS_STRING(strDir),&uFreeBytesCaller,&uTotNumBytes,&uTotNumFreeBytes);
				if (b)
				{
					// free space on disk or for the user? That is the question
					totalSpace = uTotNumBytes.QuadPart;
					res = HXR_OK;
				}
			}

			::FreeLibrary(hKernel);
		}
	}

	return res;
}

//******************************************************************************
HXBOOL CHXFileSpecUtils::IsDiskEjectable(const CHXDirSpecifier& volSpec)
{
    	HXBOOL retval = FALSE;

#if !defined(WIN32_PLATFORM_PSPC)
	CHXDirSpecifier dirRoot = volSpec.GetVolume();
	CHXString strDir = dirRoot.GetPathName();
	UINT uiType = ::GetDriveType(strDir);


	switch (uiType)
	{
	case DRIVE_REMOVABLE:
	case DRIVE_CDROM:
		retval = TRUE;
		break;
	default:
		retval = FALSE;
		break;
	}
#endif /* !defined(WIN32_PLATFORM_PSPC) */

	return retval;
}

// IsLocal returns TRUE if the file or directory is on a local volume
// (not on a server)
//******************************************************************************
HXBOOL CHXFileSpecUtils::IsDiskLocal(const CHXDirSpecifier& volSpec)
{
    HXBOOL retval = TRUE;

#if !defined(WIN32_PLATFORM_PSPC)
	CHXDirSpecifier dirRoot = volSpec.GetVolume();
	CHXString strDir = dirRoot.GetPathName();
	UINT uiType = ::GetDriveType(strDir);

	switch (uiType)
	{
	case DRIVE_FIXED:
	case DRIVE_CDROM:
	case DRIVE_RAMDISK:
	case DRIVE_REMOVABLE:
		retval = TRUE;
		break;
	default:
		retval = FALSE;
		break;
	}
#endif /* !defined(WIN32_PLATFORM_PSPC) */

	return retval;
}

// file/dir utilities
//******************************************************************** **********
HX_RESULT CHXFileSpecUtils::RemoveDir(const CHXDirSpecifier& dirSpec)
{
    if (::RemoveDirectory(OS_STRING(dirSpec.GetPathName())))
    {
        return HXR_OK;
    }

    return HXR_FAILED;
};


//******************************************************************** **********
HX_RESULT CHXFileSpecUtils::RemoveFile(const CHXFileSpecifier& fileSpec, IUnknown* pContext)
{
    if (::DeleteFile(OS_STRING(fileSpec.GetPathName())))
    {
        return HXR_OK;
    }

    return HXR_FAILED;
};


//******************************************************************************
HX_RESULT CHXFileSpecUtils::GetFileSize(const CHXFileSpecifier& fileSpec, INT64& fSize, IUnknown* pContext)
{
	CHXString strFile = fileSpec.GetPathName();

	// try to open the file
	HANDLE hFile = ::CreateFile(OS_STRING(strFile),
								GENERIC_READ,
								FILE_SHARE_READ,
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return HXR_FAIL;
	}

	DWORD dwLow,dwHigh,dwError;

	dwLow = ::GetFileSize(hFile,&dwHigh);
	if (dwLow == 0xFFFFFFFF && (dwError = GetLastError()) != NO_ERROR)
	{
		return HXR_FAIL;
	}

	fSize = dwHigh;
	fSize = fSize << 32;
	fSize |= dwLow;

	::CloseHandle(hFile);

	return HXR_OK;
}

//******************************************************************************
HX_RESULT CHXFileSpecUtils::GetDirectorySize(const CHXDirSpecifier& dirSpec, HXBOOL shouldDescend, INT64& fSize)
{
    CHXString currentDir = dirSpec.GetPathName();
    CHXString strFindFilePath = currentDir + "\\*.*";

    WIN32_FIND_DATA findFileData;
    HANDLE hFile = ::FindFirstFile(OS_STRING(strFindFilePath), &findFileData );

    HX_RESULT outResult = HXR_OK;

    while ( hFile != INVALID_HANDLE_VALUE )
    {
	if ((0 != strcmp(".", OS_STRING(findFileData.cFileName))) && 
	    (0 != strcmp("..", OS_STRING(findFileData.cFileName))) )
	{
	    if ( shouldDescend && (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
	    {
		CHXString subDirectoryPath = currentDir + "\\" + OS_STRING(findFileData.cFileName);

		CHXDirSpecifier subDirectorySpec( subDirectoryPath );
		if ( FAILED(GetDirectorySize( subDirectorySpec, shouldDescend, fSize )) )
		{
		    fSize = 0;
		    outResult = HXR_FAIL;
		    break;
		}
	    }
	    else
	    {
		INT64 dFileSize = 0;
		dFileSize = findFileData.nFileSizeHigh;
		dFileSize = dFileSize << 32;
		dFileSize |= findFileData.nFileSizeLow;
		fSize += dFileSize;
	    }
	}

	if ( 0 == ::FindNextFile( hFile, &findFileData ) )
	{
	    if ( ERROR_NO_MORE_FILES != GetLastError() )
	    {
		fSize = 0;
		outResult = HXR_FAIL;
	    }

	    break;
	}
    }

    ::FindClose( hFile );
    return HXR_OK;
}

//******************************************************************************
CHXFileSpecifier CHXFileSpecUtils::GetCurrentApplication(void)
{
	CHXFileSpecifier spec;
	char szBuf[_MAX_PATH]; /* Flawfinder: ignore */

	if (GetModuleFileName(NULL,OS_STRING2(szBuf,_MAX_PATH),_MAX_PATH))
	{
		spec = CHXFileSpecifier(szBuf); 
	}

	return spec;
}

//******************************************************************************
CHXDirSpecifier CHXFileSpecUtils::GetCurrentApplicationDir(void)
{
	CHXFileSpecifier 	appSpec;
	CHXDirSpecifier 	dirSpec;
	
	appSpec = GetCurrentApplication();
	if (appSpec.IsSet())
	{
		dirSpec = appSpec.GetParentDirectory();
	}
	return dirSpec;
}

//******************************************************************************
HXBOOL CHXFileSpecUtils::FileExists(const CHXFileSpecifier& fileSpec, IUnknown* pContext)
{
    HXBOOL bRet = FALSE;

    //struct stat statBuff;
    //ZeroInit(&statBuff);

    //if ( (stat(fileSpec.GetPathName(), &statBuff) == 0) && (statBuff.st_mode & _S_IFREG))
    //bRet = TRUE;

    ULONG32 ulAtts = ::GetFileAttributes(OS_STRING(fileSpec.GetPathName()));

    if (ulAtts != 0xFFFFFFFF && !(ulAtts & FILE_ATTRIBUTE_DIRECTORY))
	bRet = TRUE;

    return bRet;
}

//******************************************************************************
HXBOOL CHXFileSpecUtils::DirectoryExists(const CHXDirSpecifier& dirSpec)
{
	ULONG32 ulAtts = ::GetFileAttributes(OS_STRING(dirSpec.GetPathName()));

	if (ulAtts != 0xFFFFFFFF && (ulAtts & FILE_ATTRIBUTE_DIRECTORY))
		return TRUE;

	return FALSE;
}

//******************************************************************************
HX_RESULT CHXFileSpecUtils::CreateDir(const CHXDirSpecifier& dirSpec)
{
        if (::CreateDirectory(OS_STRING(dirSpec.GetPathName()),NULL))
		return TRUE;

	return FALSE;
}

//******************************************************************************
static CHXFileSpecifier GetUniqueFileSpecInternal(const CHXDirSpecifier& locationSpec, 
									const char *pszNameFirst, const char *pszTemplate, 
									const char *pszWildcard, UINT32 nStartNum);
									
const UINT32 kNumWrapValue = 9999+1;	// limit insertions to 4-digit numbers
	
CHXFileSpecifier CHXFileSpecUtils::GetUniqueFileSpec(const CHXDirSpecifier& locationSpec, 
									const char *pszNameFirst, const char *pszTemplate, 
									const char *pszWildcard)
{
	return GetUniqueFileSpecInternal(locationSpec, pszNameFirst, pszTemplate, pszWildcard, 0);
}

CHXFileSpecifier CHXFileSpecUtils::GetUniqueTempFileSpec(const CHXDirSpecifier& locationSpec, 
									const char *pszTemplate, const char *pszWildcard)
{
	CMultiplePrimeRandom 	rand(HX_GET_TICKCOUNT());
	
	UINT32 num;
	
	num = rand.GetRandomNumber();
	
	num %= kNumWrapValue;
	
	if (num == 0 || num == 1) num = 2;

	return GetUniqueFileSpecInternal(locationSpec, NULL, pszTemplate, pszWildcard, num);
}

static CHXFileSpecifier GetUniqueFileSpecInternal(const CHXDirSpecifier& locationSpec, 
									const char *pszNameFirst, const char *pszTemplate, 
									const char *pszWildcard, UINT32 nStartNum)
{
	CHXFileSpecifier 		resultFileSpec;
	
	require_return(locationSpec.IsSet(), resultFileSpec);
	require_return(pszTemplate != NULL && pszWildcard != NULL, resultFileSpec);
	require_return(pszNameFirst != NULL || nStartNum != 0, resultFileSpec);
	
	CHXFileSpecifier 	testFileSpec;
	CHXDirSpecifier 	testDirSpec;
	CHXString			strNumber;
	CHXString			strName;
	UINT32				nCurrentNum;
	
	nCurrentNum = nStartNum;

	while (1) 
	{
		// if the number is non-zero, make a string from the template;
		// if the number is zero, user the initial name string
		if (nCurrentNum == 0)
		{
			// replace the wildcard in the template with the number string
			strName = pszNameFirst;
		}
		else
		{
			// replace the wildcard in the template with the number string
			strNumber.Empty();
			strNumber.AppendULONG(nCurrentNum);

			strName = pszTemplate;
			strName.FindAndReplace(pszWildcard, strNumber);	// replace first wildcard with number string
		}
		
		
		// test if a file or directory exists with that name
		testFileSpec = locationSpec.SpecifyChildFile(strName);
		testDirSpec = locationSpec.SpecifyChildDirectory(strName);
		if (CHXFileSpecUtils::FileExists(testFileSpec)
			|| CHXFileSpecUtils::DirectoryExists(testDirSpec))
		{
			// an item already has that name, so increment & wrap the number
			nCurrentNum++;
			nCurrentNum %= kNumWrapValue;
			
			// don't use 0 again, and skip 1 since "MyFile2.txt" logically follows "MyFile.txt"
			if (nCurrentNum == 0 || nCurrentNum == 1) 
			{
				nCurrentNum = 2; 
			}
			
			// a quick sanity check
			if (nCurrentNum == nStartNum)
			{
				check(!"GetUniqueFileSpecInternal number wrapped");
				break;
			}
		}
		else
		{
			// the name is unique
			resultFileSpec = testFileSpec;
			break;
		}
		
	} // while

	return resultFileSpec;
}
//******************************************************************************
CHXDirSpecifier CHXFileSpecUtils::GetSystemTempDirectory()
{
	char szBuf[MAX_PATH] = ""; /* Flawfinder: ignore */

#if !defined(WIN32_PLATFORM_PSPC)
	::GetTempPath(MAX_PATH,szBuf);
#endif /* !defined(WIN32_PLATFORM_PSPC) */

	CHXDirSpecifier retSpec(szBuf);
	return retSpec;
}




HXBOOL CHXFileSpecUtils::MakeNameLegal(char *pszName)
{
	const char *badChars = "\\/:*?\"<>|";
	const char replacementChar = '-';
	const long maxNameLength = 255;
	char *pCurrChar = pszName;

	HXBOOL bChanged;
	
	require_nonnull_return(pszName, FALSE);
	
	bChanged = FALSE;

	// replace any illegal characters
	while (*pCurrChar)
	{
		if (strchr(badChars, *pCurrChar))
		{
			*pCurrChar = replacementChar;
			bChanged = TRUE;
		}
		pCurrChar = HXGetNextChar(pCurrChar);

		// be sure the name isn't too long
		if (pCurrChar - pszName >= maxNameLength)
		{
			if (pCurrChar - pszName >= maxNameLength + 1)
				pCurrChar = HXGetPrevChar(pszName, pCurrChar);
			*pCurrChar = 0;
			bChanged = TRUE;
		}
	}
	
	return bChanged;
}

//******************************************************************************
CHXDirSpecifier 
CHXFileSpecUtils::GetAppDataDir(const char* szAppName)
{
    CHXDirSpecifier dirResult;

    CHXString strPath;

    HINSTANCE hShell = ::LoadLibrary(OS_STRING("shell32.dll"));
    FPSHGetSpecialFolderLocation fpSHGetSpecialFolderLocation = NULL;
    FPSHGetPathFromIDList fpSHGetPathFromIDList = NULL;
    FPSHGetMalloc fpSHGetMalloc = NULL;
    if (hShell)
    {
	fpSHGetSpecialFolderLocation = (FPSHGetSpecialFolderLocation) GetProcAddress(hShell,OS_STRING("SHGetSpecialFolderLocation"));
	fpSHGetPathFromIDList = (FPSHGetPathFromIDList) GetProcAddress(hShell, OS_STRING("SHGetPathFromIDListA"));
	// just in case
	if (!fpSHGetPathFromIDList)
	{
	    fpSHGetPathFromIDList = (FPSHGetPathFromIDList) GetProcAddress(hShell, OS_STRING("SHGetPathFromIDList"));
	}
	fpSHGetMalloc = (FPSHGetMalloc) GetProcAddress(hShell, OS_STRING("SHGetMalloc"));
    }

    if (!fpSHGetSpecialFolderLocation	||
	!fpSHGetPathFromIDList		||
	!fpSHGetMalloc)
    {
	goto exit;
    }

#if !defined(WIN32_PLATFORM_PSPC)
    ITEMIDLIST* pidl;
    pidl = NULL;
    if(fpSHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl) == NOERROR)
    {
	HX_ASSERT(pidl);
	fpSHGetPathFromIDList(pidl, strPath.GetBuffer(_MAX_PATH + 1));
	
	IMalloc* pMalloc = NULL;
	if (SUCCEEDED(fpSHGetMalloc(&pMalloc)))
	{
	    pMalloc->Free(pidl);
	    pMalloc->Release();
	    pMalloc = NULL;
	    pidl = NULL;
	}

	strPath.ReleaseBuffer();
    }
    else
    {
	// In Win95 OSR2 SHGetSpecialFolderLocation() fails on CSIDL_APPDATA.
	HKEY hKey = NULL;
	if(RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion", &hKey) == ERROR_SUCCESS)
	{
	    DWORD nValueType = REG_NONE;
	    DWORD bufSize = _MAX_PATH;
	    RegQueryValueEx(hKey, OS_STRING("ProgramFilesDir"), 
			    NULL, &nValueType, 
			    (Byte *)strPath.GetBuffer(_MAX_PATH + 1), 
			    &bufSize);
	    strPath.ReleaseBuffer();
	    RegCloseKey(hKey);
	}
    }
#endif /* !defined(WIN32_PLATFORM_PSPC) */

    if(!strPath.IsEmpty())
    {
	dirResult = strPath;
	if(!DirectoryExists(dirResult))
	    CreateDir(dirResult);

	dirResult = dirResult.SpecifyChildDirectory("Real");
	if(!DirectoryExists(dirResult))
	    CreateDir(dirResult);

	if(szAppName)
	{
	    dirResult = dirResult.SpecifyChildDirectory(szAppName);
	    if(!DirectoryExists(dirResult))
		CreateDir(dirResult);
	}
    }

exit:
    if (hShell)
    {
	::FreeLibrary(hShell);
    }

    return dirResult;
}

//******************************************************************************

HX_RESULT CHXFileSpecUtils::ReadBinaryFile(const CHXFileSpecifier& fileSpec, IHXBuffer*& pOutBuffer)
{
    if(!fileSpec.IsSet())
    {
        return HXR_INVALID_PARAMETER;
    }

    CHXString strFilename = fileSpec.GetPathName();
    FILE* pFile = NULL;
    struct stat aStatBuf;
    int nRet;
    HX_RESULT retVal = HXR_OK;

    pFile = fopen(strFilename, "rb");
    if(!pFile)
    {
        if(errno == EACCES || errno == ENOENT)
        {
            retVal = HXR_DOC_MISSING;
        }
        else
        {
            retVal = HXR_FAIL;
        }                        
    }

    if(SUCCEEDED(retVal))
    {
        nRet = fstat((int)fileno(pFile), &aStatBuf);
        if(nRet != 0)
        {
            retVal = HXR_FAIL;
        }
    }

    if(SUCCEEDED(retVal))
    {
        retVal = pOutBuffer->SetSize(aStatBuf.st_size);
    }

    if(SUCCEEDED(retVal))
    {
        void* pBufData = pOutBuffer->GetBuffer();
        int nRead;

        nRead = fread(pBufData, 1, aStatBuf.st_size, pFile);

        HX_ASSERT(nRead == aStatBuf.st_size);
    }

    // Clean up
    if(pFile)
    {
        fclose(pFile);
    }

    return retVal;
}

HX_RESULT CHXFileSpecUtils::ReadTextFile(const CHXFileSpecifier& fileSpec, CHXString& outStr)
{
    HX_RESULT res = HXR_FAIL;
    FILE* f = NULL;
    long cbFile = 0;
    char* szFileData = NULL;

    if(!fileSpec.IsSet())
    {
        res = HXR_INVALID_PARAMETER;
        goto bail;
    }

    f = fopen(fileSpec.GetPathName(), "r+t");
    if(!f)
        goto bail;

    fseek(f, 0, SEEK_END);
    cbFile = ftell(f);
    if(!cbFile)
        goto bail;

    szFileData = outStr.GetBuffer(cbFile + 1);
    memset(szFileData, 0, cbFile + 1);
    fseek(f, 0, SEEK_SET);
    fread(szFileData, 1, cbFile, f);
    outStr.ReleaseBuffer();
    res = HXR_OK;

bail:
    if(f)
        fclose(f);
    return res;
}

HX_RESULT CHXFileSpecUtils::WriteBinaryFile(CHXFileSpecifier& fileSpec, IHXBuffer* inBuffer, HXBOOL bReplaceExistingFile)
{
    HX_RESULT res = HXR_FAIL;

    if(!fileSpec.IsSet())
    {
        return HXR_INVALID_PARAMETER;
    }

    HXBOOL bExists = FileExists(fileSpec);
    if (!bReplaceExistingFile && bExists)
    {
        return HXR_FILE_EXISTS;
    }

    FILE* pFile = fopen(fileSpec.GetPathName(), "wb");
    if(pFile)
    {
        int nWritten;
        int nBufferSize = inBuffer->GetSize();
        void* pBufferData = inBuffer->GetBuffer();

        nWritten = fwrite(pBufferData, 1, nBufferSize, pFile);
        fclose(pFile);
    }

    return res;
}

HX_RESULT CHXFileSpecUtils::WriteTextFile(CHXFileSpecifier& fileSpec, const CHXString& inStr, HXBOOL bReplaceExistingFile)
{
    // the first parameter is not const because on the Mac it can change during the call
    HXBOOL bExists = FileExists(fileSpec);
    if (!bReplaceExistingFile && bExists) 
        return HXR_FILE_EXISTS;

    if(bReplaceExistingFile && bExists)
        RemoveFile(fileSpec);

    HX_RESULT res = HXR_FAIL;

    FILE* f = fopen(fileSpec.GetPathName(), "w");
    if(f)
    {
        if(fputs(inStr, f) >= 0)
            res = HXR_OK;
        fclose(f);
    }
    return res;
}

HX_RESULT CHXFileSpecUtils::RenameMoveFile(CHXFileSpecifier& fileSpec, const char* pNewNameIfAny, 
                                           const CHXDirSpecifier* pNewDirectoryIfAny)
{
    HX_RESULT       res;
    CHXDirSpecifier     fileDir, destDir;
    CHXFileSpecifier    interimFileSpec;
    CHXString       strNewName;
    HXBOOL            bRenaming, bMoving;

    res = HXR_FAIL;

    require_return(fileSpec.IsSet(), HXR_INVALID_PARAMETER);

    fileDir = fileSpec.GetParentDirectory();

    if (pNewDirectoryIfAny)
    {
        destDir = *pNewDirectoryIfAny;
        bMoving = (destDir != fileDir) ? TRUE : FALSE;
    }
    else
    {
        destDir = fileDir;
        bMoving = FALSE;
    }

    if (pNewNameIfAny)
    {
        strNewName = pNewNameIfAny;
        bRenaming = (strNewName != fileSpec.GetName()) ? TRUE : FALSE;
    }
    else
    {
        strNewName = fileSpec.GetName();
        bRenaming = FALSE;
    }

    // ensure there's not already something there with this name
    CHXFileSpecifier specTestDest = destDir.SpecifyChildFile(strNewName);
    require( (!CHXFileSpecUtils::FileExists(specTestDest)), Bail);

    if ((bRenaming || bMoving) && specTestDest.IsSet() && !CHXFileSpecUtils::FileExists(specTestDest))
    {
	if(::MoveFile(OS_STRING((const char*)(fileSpec.GetPathName())), OS_STRING((const char*)(specTestDest.GetPathName()))))
            res = HXR_OK;
    }

    if (SUCCEEDED(res))
    {
        fileSpec = specTestDest;
    }
Bail:
    return res;
}
