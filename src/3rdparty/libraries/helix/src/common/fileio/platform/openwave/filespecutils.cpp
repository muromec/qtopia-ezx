/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: filespecutils.cpp,v 1.7 2006/05/19 17:19:54 ping Exp $
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


#define NFS_SUPER_MAGIC 0x6969
#define SMB_SUPER_MAGIC 0x517B

//////////////////////////////////////////////////////////
//
// Utility base class -- CHXFileSpecUtils
//
//////////////////////////////////////////////////////////

//******************************************************************************

//#if defined(_FREEBSD) || defined (_OSF1)
//#include <sys/mount.h>
//#else
//#include <sys/vfs.h>
//#endif

#include "op_fs.h"
#include <sys/stat.h>
//#include <dirent.h>
#include <sys/types.h>

#define ONEGIG (1<<30)

#define STATFS_PATH_TYPE const char*

#define HXStatfsStruct        struct OpFsStatStruct
#define HXStatfs(path,sf)     OpFsStat((STATFS_PATH_TYPE)(path),(sf))


HX_RESULT CHXFileSpecUtils::GetFreeSpaceOnDisk(const CHXDirSpecifier& volSpec, INT64& freeSpace)
{
    HX_RESULT result = HXR_NOTIMPL;
    const char *szPath = volSpec.GetPathName();
    
    HXStatfsStruct sf;
    if (kOpFsErrAny != HXStatfs(szPath, &sf))
    {

	    freeSpace = (UINT64)sf.size;
        result = HXR_OK;
    }
    else
    {
		perror("statfs");
		freeSpace = ONEGIG; 
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
    if (kOpFsErrAny != HXStatfs(szPath, &sf))
    {
        totalSpace = (UINT64)sf.size;
        result = HXR_OK;
    }
    else
    {
		perror("statfs");
		totalSpace = ONEGIG; 
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
    return TRUE;
}

// file/dir utilities
//******************************************************************** **********
HX_RESULT CHXFileSpecUtils::RemoveDir(const CHXDirSpecifier& dirSpec)
{
    return HXR_NOTIMPL;
};


//******************************************************************** **********
HX_RESULT CHXFileSpecUtils::RemoveFile(const CHXFileSpecifier& fileSpec, IUnknown* pContext)
{
    if (kOpFsErrAny != OpFsRemove(fileSpec.GetPathName()))
    {
        return HXR_OK;
    }

    return HXR_FAILED;
};


//******************************************************************************
HX_RESULT CHXFileSpecUtils::GetFileSize(const CHXFileSpecifier& fileSpec, INT64& fSize, IUnknown* pContext)
{
    HX_RESULT result = HXR_FAIL;
 
    HXStatfsStruct sf;
    if (kOpFsErrAny != HXStatfs(fileSpec.GetPathName(), &sf))
    {
        fSize = sf.size;
        result = HXR_OK;
    }
    return result;
}

//******************************************************************************
HX_RESULT CHXFileSpecUtils::GetDirectorySize(const CHXDirSpecifier& dirSpec, HXBOOL shouldDescend, INT64& fSize)
{
	return HXR_NOTIMPL;
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
 
	HXStatfsStruct sf;
    if (kOpFsErrAny != HXStatfs(fileSpec.GetPathName(), &sf))
	{
       result = TRUE;
    }
    return result;
}

//******************************************************************************
HXBOOL CHXFileSpecUtils::DirectoryExists(const CHXDirSpecifier& dirSpec)
{
    HXBOOL result = FALSE;
 
	HXStatfsStruct sf;
    if (kOpFsErrAny != HXStatfs(dirSpec.GetPathName(), &sf))
	{
       result = TRUE;
    }
    return result;
}

//******************************************************************************
HX_RESULT CHXFileSpecUtils::CreateDir(const CHXDirSpecifier& dirSpec)
{        
	return HXR_NOTIMPL;;
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
