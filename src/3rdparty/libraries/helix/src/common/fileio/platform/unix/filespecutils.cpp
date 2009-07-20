/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: filespecutils.cpp,v 1.18 2006/05/19 17:19:54 ping Exp $
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

#include "filespecutils.h"
#include "hxtick.h"
#include "hxrand.h"
#include "dbcs.h"

#include <stdio.h>
#include <errno.h>

#define NFS_SUPER_MAGIC 0x6969
#define SMB_SUPER_MAGIC 0x517B

//////////////////////////////////////////////////////////
//
// Utility base class -- CHXFileSpecUtils
//
//////////////////////////////////////////////////////////

//******************************************************************************

#if defined(_FREEBSD) || defined(_OPENBSD) || defined(_NETBSD) || defined (_OSF1) || defined(_MAC_UNIX)
#include <sys/mount.h>
#else
#include <sys/vfs.h>
#endif
#if defined(_SOLARIS) || defined(_LSB_)
#include <sys/statfs.h>
#include <sys/statvfs.h>
#endif

#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>

#define ONEGIG (1<<30)

#if defined(_OSF1) || defined(_AIX)
#define STATFS_PATH_TYPE char*
#else
#define STATFS_PATH_TYPE const char*
#endif

#if defined(_SOLARIS) || defined(_LSB_)
#define HXStatfsStruct        struct statvfs
#define HXStatfs(path,sf)     statvfs((STATFS_PATH_TYPE)(path),(sf))
#define HXStatfsBlockSize(sf) ((sf).f_blocks)
#else
#define HXStatfsStruct        struct statfs
#define HXStatfs(path,sf)     statfs((STATFS_PATH_TYPE)(path),(sf))
#define HXStatfsBlockSize(sf) ((sf).f_bsize)
#endif

HX_RESULT CHXFileSpecUtils::GetFreeSpaceOnDisk(const CHXDirSpecifier& volSpec, INT64& freeSpace)
{
    HX_RESULT result = HXR_NOTIMPL;
    const char *szPath = volSpec.GetPathName();
    
    HXStatfsStruct sf;
    if (-1 != HXStatfs(szPath, &sf))
    {
	if (geteuid()) 
	{
	    freeSpace = (UINT64)sf.f_bavail * (UINT64)HXStatfsBlockSize(sf);
	}
	else 
	{
	    freeSpace = (UINT64)sf.f_bfree * (UINT64)HXStatfsBlockSize(sf);
	}
        result = HXR_OK;
    }
    else
    {
	perror("statfs");
	freeSpace = (INT64)ONEGIG; 
    }

    return result;
}

//******************************************************************************
HX_RESULT CHXFileSpecUtils::GetTotalSpaceOnDisk(const CHXDirSpecifier& volSpec, 
						INT64& totalSpace)
{ 
    HX_RESULT result = HXR_NOTIMPL;
    const char *szPath = volSpec.GetPathName();
    
    HXStatfsStruct sf;
    if (-1 != HXStatfs(szPath, &sf))
    {
        totalSpace = (UINT64)sf.f_blocks * (UINT64)HXStatfsBlockSize(sf);
        result = HXR_OK;
    }
    else
    {
	perror("statfs");
	totalSpace = (INT64)ONEGIG; 
    }

    return result;
}

//******************************************************************************
HXBOOL CHXFileSpecUtils::IsDiskEjectable(const CHXDirSpecifier& volSpec)
{
    return FALSE;
}

// IsLocal returns TRUE if the file or directory is on a local volume
// (not on a server)
//******************************************************************************


/* Include correct headers to get more fs types below... */

HXBOOL CHXFileSpecUtils::IsDiskLocal(const CHXDirSpecifier& volSpec)
{
    HXBOOL result = TRUE;
    const char *szPath = volSpec.GetPathName();
    
    HXStatfsStruct sf;
    if (-1 != HXStatfs(szPath, &sf))
    {
#if !defined(_LSB_)
#if defined(_SOLARIS)
        if (strcmp("nfs", sf.f_basetype) == 0)
        {
            result = FALSE;
        }
#elif defined(_OPENBSD)
        if (strcmp("nfs", sf.f_fstypename) == 0) //XXX untested
        {
            result = FALSE;
        }
#else
        if (sf.f_type == NFS_SUPER_MAGIC ||
            sf.f_type == SMB_SUPER_MAGIC)
        {
            result = FALSE;
        }
#endif
#endif
    }
    else
    {
	perror("statfs");
    }
    return result;
}

// file/dir utilities
//******************************************************************** **********
HX_RESULT CHXFileSpecUtils::RemoveDir(const CHXDirSpecifier& dirSpec)
{
    if (-1 != rmdir(dirSpec.GetPathName()))
    {
        return HXR_OK;
    }

    return HXR_FAILED;
};


//******************************************************************** **********
HX_RESULT CHXFileSpecUtils::RemoveFile(const CHXFileSpecifier& fileSpec, IUnknown* pContext)
{
    if (-1 != unlink(fileSpec.GetPathName()))
    {
        return HXR_OK;
    }

    return HXR_FAILED;
};


//******************************************************************************
HX_RESULT CHXFileSpecUtils::GetFileSize(const CHXFileSpecifier& fileSpec, INT64& fSize, IUnknown* pContext)
{
    HX_RESULT result = HXR_FAIL;
    struct stat st;
    if (-1 != stat(fileSpec.GetPathName(), &st))
    {
        fSize = st.st_size;
        result = HXR_OK;
    }
    return result;
}

//******************************************************************************
HX_RESULT CHXFileSpecUtils::GetDirectorySize(const CHXDirSpecifier& dirSpec, HXBOOL shouldDescend, INT64& fSize)
{
    DIR *dir = opendir(dirSpec.GetPathName());
    HX_RESULT result = HXR_FAIL;
    while (struct dirent *dirent_ = readdir(dir))
    {
        result = HXR_OK;
        struct stat st;
        if (-1 != stat(dirent_->d_name, &st))
        {
            if (S_ISDIR(st.st_mode))
            {
                if (shouldDescend)
                {
                    GetDirectorySize(CHXDirSpecifier(dirent_->d_name), TRUE, fSize);
                }
            }
            else
            {
                fSize = st.st_size;
            }
        }
    }
    if (dir)
    {
        closedir(dir);
    }
    return result;
}

//******************************************************************************
CHXFileSpecifier CHXFileSpecUtils::GetCurrentApplication(void)
{
    /* AFAIK there is no way to implement this.  Prove me wrong */
    CHXFileSpecifier spec;
    return spec;
}

//******************************************************************************
CHXDirSpecifier CHXFileSpecUtils::GetCurrentApplicationDir(void)
{
    /* AFAIK there is no way to implement this.  Prove me wrong */
    CHXDirSpecifier 	dirSpec;
    return dirSpec;
}

//******************************************************************************
HXBOOL CHXFileSpecUtils::FileExists(const CHXFileSpecifier& fileSpec, IUnknown* pContext)
{
    HXBOOL result = FALSE;
    struct stat st;
    if (-1 != stat(fileSpec.GetPathName(), &st))
    {
        result = !S_ISDIR(st.st_mode);
    }
    return result;
}

//******************************************************************************
HXBOOL CHXFileSpecUtils::DirectoryExists(const CHXDirSpecifier& dirSpec)
{
    HXBOOL result = FALSE;
    struct stat st;
    if (-1 != stat(dirSpec.GetPathName(), &st))
    {
        result = S_ISDIR(st.st_mode);
    }
    return result;
}

//******************************************************************************
HX_RESULT CHXFileSpecUtils::CreateDir(const CHXDirSpecifier& dirSpec)
{
	if (-1 != mkdir(dirSpec.GetPathName(), 0700))
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
    return CHXDirSpecifier(getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp");
}


HXBOOL CHXFileSpecUtils::MakeNameLegal(char *pszName)
{
    const char *badChars = "\\/:*?\"<>| ][()'";
    const char replacementChar = '_';
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
    return CHXDirSpecifier("/huh");
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
                nRet = fstat(fileno(pFile), &aStatBuf);
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
        if(!fileSpec.IsSet())
        {
                return HXR_INVALID_PARAMETER;
        }

        CHXString strFilename = fileSpec.GetPathName();
        FILE* pFile = NULL;
        struct stat aStatBuf;
        int nRet;
        HX_RESULT retVal = HXR_OK;
        
        pFile = fopen(strFilename, "r");
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
                nRet = fstat(fileno(pFile), &aStatBuf);
                if(nRet != 0)
                {
                        retVal = HXR_FAIL;
                }
        }
        
        if(SUCCEEDED(retVal))
        {
                if(aStatBuf.st_size == 0)
                {
                        // CHXString::GetBufferSetLength doesn't like strings of length 0,
                        // handle this case here.
                        char* szBufData = outStr.GetBufferSetLength(1);
                        
                        *szBufData = '\0';
                }
                else
                {
                        char* szBufData = outStr.GetBufferSetLength(aStatBuf.st_size);
                        int nRead;

                        nRead = fread(szBufData, 1, aStatBuf.st_size, pFile);

                        HX_ASSERT(nRead == aStatBuf.st_size);
                }
        }
        
        // Clean up
        if(pFile)
        {
                fclose(pFile);
        }
        
	return retVal;
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

                nWritten = fwrite(pBufferData, nBufferSize, 1, pFile);
                if(nWritten == 1)
                {
                        res = HXR_OK;
                }
                fclose(pFile);
        }
        
	return res;
}

HX_RESULT CHXFileSpecUtils::WriteTextFile(CHXFileSpecifier& fileSpec, const CHXString& inStr, HXBOOL bReplaceExistingFile)
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

        FILE* pFile = fopen(fileSpec.GetPathName(), "w");
        if(pFile)
        {
                if(fputs(inStr, pFile) >= 0)
                {
                        res = HXR_OK;
                }
                fclose(pFile);
        }
        
	return res;
}
